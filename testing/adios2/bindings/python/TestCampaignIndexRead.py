#!/usr/bin/env python
# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
# SPDX-License-Identifier: Apache-2.0
"""Image/text data reads through a campaign index, without external fixtures."""

import json
import sqlite3
import tempfile
import unittest
import zlib
from contextlib import closing
from pathlib import Path

import adios2
import numpy as np
from TestCampaignRead import SCHEMA


INDEX_SCHEMA = """
CREATE TABLE info(id TEXT, name TEXT, version TEXT, modtime INT);
INSERT INTO info VALUES ('ACX','test','0.1',0);
CREATE TABLE variabletypes(typeid INT, name TEXT);
INSERT INTO variabletypes VALUES (1,'char'), (2,'uint8_t');
CREATE TABLE datasettypes(typeid INT, name TEXT);
INSERT INTO datasettypes VALUES (3,'IMAGE'), (4,'TEXT');
CREATE TABLE archives(archiveid INT, name TEXT);
INSERT INTO archives VALUES (1,'store/data.aca');
CREATE TABLE datasets(datasetid INT, archiveid INT, dsid INT, name TEXT, type INT);
CREATE TABLE variables(archiveid INT, datasetid INT, name TEXT, type INT, steps INT,
                       min TEXT, max TEXT, ndim INT, shape TEXT, value TEXT);
CREATE TABLE attributes(datasetid INT, name TEXT, type INT, value TEXT);
"""


class TestCampaignIndexRead(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.index = self.root / "data.acx"
        self.archive = self.root / "data.aca"
        self.payload = b"\x89PNG\r\n\x1a\n\x00\xff" * 17

    def fixture(self, image=False, embedded=False, compressed=False, tar=False):
        payload = self.payload if image else b"<adios-config>\nhello\n</adios-config>\n"
        self.payload = payload
        dataset = "image.png" if image else "config.xml"
        suffix = "/256x256" if image else ""
        self.name = "store/data.aca/" + dataset + suffix
        with closing(sqlite3.connect(self.index)) as db, db:
            db.executescript(INDEX_SCHEMA)
            db.execute("INSERT INTO datasets VALUES (1,1,13,?,?)", (dataset, 3 if image else 4))
            db.execute(
                "INSERT INTO variables VALUES (1,1,?,?,1,NULL,NULL,1,?,NULL)",
                ("256x256" if image else "", 2 if image else 1, str(len(payload))),
            )
        with closing(sqlite3.connect(self.archive)) as db, db:
            db.executescript(SCHEMA)
            db.execute("INSERT INTO info VALUES ('ACA','test','0.8',0)")
            db.execute("INSERT INTO host VALUES ('fixture-local','localhost',0,0,'')")
            db.execute("INSERT INTO directory VALUES (1,?,0,0)", (str(self.root),))
            db.execute(
                "INSERT INTO dataset(rowid,name,uuid,modtime,deltime,fileformat,tsid,tsorder) "
                "VALUES (13,?,?,0,0,?,0,0)",
                (dataset, "uuid", "IMAGE" if image else "TEXT"),
            )
            db.execute(
                "INSERT INTO replica VALUES (13,1,1,?,?,0,0,0,?)",
                (1 if tar else 0, dataset, len(payload)),
            )
            if image:
                db.execute("INSERT INTO resolution VALUES (1,256,256)")
            if embedded:
                blob = zlib.compress(payload) if compressed else payload
                db.execute(
                    "INSERT INTO file VALUES (1,?,?, ?,?,0,?,?)",
                    (dataset, int(compressed), len(payload), len(blob), "", blob),
                )
                db.execute("INSERT INTO repfiles VALUES (1,1)")
                # A different local copy must not override the embedded object.
                (self.root / dataset).write_bytes(b"x" * len(payload))
            elif tar:
                (self.root / "data.tar").write_bytes(b"padding" + payload + b"trailer")
                db.execute("INSERT INTO archive VALUES (1,'data.tar','fs',NULL)")
                db.execute("INSERT INTO archiveidx VALUES (1,1,?,0,7,?)", (dataset, len(payload)))
            else:
                (self.root / dataset).write_bytes(payload)

    def open(self, mode=adios2.Mode.ReadRandomAccess):
        adios = adios2.Adios()
        io = adios.declare_io("index")
        io.set_parameter("hostname", "fixture-local")
        engine = io.open(str(self.index), mode)
        return adios, io, engine

    def test_sources(self):
        for image, embedded, compressed, tar in [
            (False, True, False, False),
            (True, True, False, False),
            (False, True, True, False),
            (True, True, True, False),
            (False, False, False, False),
            (True, False, False, False),
            (False, False, False, True),
            (True, False, False, True),
        ]:
            with self.subTest(image=image, embedded=embedded, compressed=compressed, tar=tar):
                self.index.unlink(missing_ok=True)
                self.archive.unlink(missing_ok=True)
                self.fixture(image, embedded, compressed, tar)
                before = self.archive.read_bytes()
                adios, io, engine = self.open()
                var = io.inquire_variable(self.name)
                data = np.zeros(len(self.payload), dtype=np.uint8 if image else np.int8)
                engine.get(var, data)
                self.assertEqual(data.tobytes(), self.payload)
                var.set_selection([[3], [11]])
                part = np.zeros(11, dtype=data.dtype)
                engine.get(var, part)
                self.assertEqual(part.tobytes(), self.payload[3:14])
                engine.close()
                self.assertEqual(self.archive.read_bytes(), before)

    def test_parsed_text_scalars(self):
        # JSON fields belong to TEXT datasets but are typed scalar metadata,
        # rather than byte streams. They must work even without the archive.
        with closing(sqlite3.connect(self.index)) as db, db:
            db.executescript(INDEX_SCHEMA)
            db.execute("INSERT INTO variabletypes VALUES (3,'string'), (4,'double'), (5,'int8_t')")
            db.execute("INSERT INTO datasets VALUES (1,1,13,'settings.json',4)")
            for name, typeid, value in [
                ("Adios_Config", 3, "adios2.xml"),
                ("F", 4, 0.01),
                ("Checkpoint", 5, True),
            ]:
                db.execute(
                    "INSERT INTO variables VALUES (1,1,?,?,1,NULL,NULL,0,?,?)",
                    (name, typeid, "", json.dumps(value)),
                )
        adios, io, engine = self.open()
        try:
            prefix = "store/data.aca/settings.json/"
            self.assertEqual(engine.get(io.inquire_variable(prefix + "Adios_Config")), "adios2.xml")
            for name, dtype, expected in [("F", np.float64, 0.01), ("Checkpoint", np.int8, 1)]:
                var = io.inquire_variable(prefix + name)
                for mode in (adios2.Mode.Sync, adios2.Mode.Deferred):
                    data = np.zeros(1, dtype)
                    engine.get(var, data, mode)
                    engine.perform_gets()
                    self.assertEqual(data[0], expected)
        finally:
            engine.close()

    def test_deferred_selections(self):
        self.fixture(image=True, embedded=True)
        adios, io, engine = self.open()
        var = io.inquire_variable(self.name)
        a, b = np.zeros(4, np.uint8), np.zeros(5, np.uint8)
        var.set_selection([[2], [4]])
        engine.get(var, a, adios2.Mode.Deferred)
        var.set_selection([[10], [5]])
        engine.get(var, b, adios2.Mode.Deferred)
        self.assertFalse(a.any())
        engine.perform_gets()
        self.assertEqual(a.tobytes(), self.payload[2:6])
        self.assertEqual(b.tobytes(), self.payload[10:15])
        engine.perform_gets()
        engine.close()

    def test_end_step_and_close(self):
        self.fixture(embedded=True)
        for streaming in (False, True):
            adios, io, engine = self.open(
                adios2.Mode.Read if streaming else adios2.Mode.ReadRandomAccess
            )
            if streaming:
                self.assertEqual(engine.begin_step(), adios2.StepStatus.OK)
            var = io.inquire_variable(self.name)
            data = np.zeros(len(self.payload), np.int8)
            engine.get(var, data, adios2.Mode.Deferred)
            if streaming:
                engine.end_step()
            engine.close()
            self.assertEqual(data.tobytes(), self.payload)


if __name__ == "__main__":
    unittest.main()
