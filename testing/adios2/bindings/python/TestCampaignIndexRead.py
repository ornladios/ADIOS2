#!/usr/bin/env python
# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
# SPDX-License-Identifier: Apache-2.0
"""Dataset and image/text reads through a campaign index, without external fixtures."""

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

    def open(self, mode=adios2.Mode.ReadRandomAccess, parameters=None):
        adios = adios2.Adios()
        io = adios.declare_io("index")
        io.set_parameter("hostname", "fixture-local")
        if parameters:
            io.set_parameters(parameters)
        engine = io.open(str(self.index), mode)
        return adios, io, engine

    def metadata_fixture(self):
        # All entries share one archive and dataset. No archive exists: Open
        # must use only the index, splitting work within this single dataset.
        with closing(sqlite3.connect(self.index)) as db, db:
            db.executescript(INDEX_SCHEMA)
            db.execute("INSERT INTO variabletypes VALUES (3,'int32_t'),(4,'string')")
            db.execute("INSERT INTO datasets VALUES (1,1,13,'one.bp',4)")
            for i in range(37):
                db.execute(
                    "INSERT INTO variables VALUES (1,1,?,3,1,?,?,0,'',?)",
                    (f"v{i:03}", str(i), str(i), str(i)),
                )
                db.execute(
                    "INSERT INTO attributes VALUES (1,?,?,?)",
                    (
                        f"/a{i:03}",
                        3 if i % 2 == 0 else 4,
                        json.dumps([i, i + 1] if i % 2 == 0 else f"text {i}"),
                    ),
                )
            db.execute("INSERT INTO variables VALUES (1,1,'array',3,3,'3','8',2,'2,3',NULL)")

    def test_metadata_batches(self):
        self.metadata_fixture()
        before = self.index.read_bytes()
        prefix = "store/data.aca/one.bp/"
        for threads, batch in [(1, 1), (4, 1), (3, 7), (4, 32), (4, 4096)]:
            with self.subTest(threads=threads, batch=batch):
                adios, io, engine = self.open(
                    parameters={"MetadataThreads": str(threads), "metadatabatchsize": str(batch)}
                )
                try:
                    self.assertEqual(len(io.available_variables()), 38)
                    self.assertEqual(len(io.available_attributes()), 37)
                    self.assertEqual(engine.steps(), 3)
                    for i in range(37):
                        var = io.inquire_variable(prefix + f"v{i:03}")
                        data = np.zeros(1, np.int32)
                        engine.get(var, data, adios2.Mode.Sync)
                        self.assertEqual(data[0], i)
                        attr = io.inquire_attribute(prefix + f"/a{i:03}")
                        if i % 2 == 0:
                            np.testing.assert_array_equal(attr.data(), [i, i + 1])
                        else:
                            self.assertEqual(attr.data_string(), f"text {i}")
                    array = io.inquire_variable(prefix + "array")
                    self.assertEqual(array.shape(), [2, 3])
                    self.assertEqual(array.steps(), 3)
                    summary = io.available_variables()[prefix + "array"]
                    self.assertEqual(float(summary["Min"]), 3)
                    self.assertEqual(float(summary["Max"]), 8)
                finally:
                    engine.close()
        self.assertEqual(self.index.read_bytes(), before)

    def test_metadata_multiple_sources(self):
        self.metadata_fixture()
        with closing(sqlite3.connect(self.index)) as db, db:
            db.execute("INSERT INTO archives VALUES (7,'store/second.aca')")
            db.execute("INSERT INTO datasets VALUES (91,7,100,'other.json',4)")
            db.execute("INSERT INTO variables VALUES (7,91,'v000',3,1,'99','99',0,'','99')")
            db.execute("INSERT INTO attributes VALUES (91,'/a000',3,'[17]')")
        adios, io, engine = self.open(parameters={"MetadataThreads": "4", "MetadataBatchSize": "7"})
        try:
            self.assertEqual(len(io.available_variables()), 39)
            self.assertEqual(len(io.available_attributes()), 38)
            prefix = "store/second.aca/other.json/"
            data = np.zeros(1, np.int32)
            engine.get(io.inquire_variable(prefix + "v000"), data, adios2.Mode.Sync)
            self.assertEqual(data[0], 99)
            np.testing.assert_array_equal(io.inquire_attribute(prefix + "/a000").data(), [17])
        finally:
            engine.close()

    def test_empty_and_attribute_only_batches(self):
        self.metadata_fixture()
        with closing(sqlite3.connect(self.index)) as db, db:
            db.execute("DELETE FROM variables")
        for empty in (False, True):
            if empty:
                with closing(sqlite3.connect(self.index)) as db, db:
                    db.execute("DELETE FROM attributes")
            adios, io, engine = self.open(
                parameters={"MetadataThreads": "4", "MetadataBatchSize": "3"}
            )
            self.assertEqual(len(io.available_variables()), 0)
            self.assertEqual(len(io.available_attributes()), 0 if empty else 37)
            self.assertEqual(engine.steps(), 0)
            engine.close()

    def test_metadata_loading_errors(self):
        # Fail in a worker, in the producer with batches in flight, and during
        # a serial merge. Every path must join the pool and release SQLite.
        cases = [
            ("UPDATE variables SET ndim=2 WHERE rowid=37", "unsupported shape"),
            ("UPDATE variables SET min='bad' WHERE rowid=37", None),
            ("UPDATE attributes SET value='invalid json' WHERE rowid=37", None),
            ("UPDATE variables SET name='v000' WHERE rowid=37", "already defined"),
            (
                "UPDATE attributes SET name='/a000' WHERE rowid=37",
                "already defined|duplicate attribute",
            ),
            ("UPDATE variables SET datasetid=999 WHERE rowid=37", None),
            ("DROP TABLE attributes", "no such table"),
        ]
        for sql, message in cases:
            for threads, batch in [(1, 3), (4, 3), (4, 4096)]:
                with self.subTest(sql=sql, threads=threads, batch=batch):
                    self.index.unlink(missing_ok=True)
                    self.metadata_fixture()
                    with closing(sqlite3.connect(self.index)) as db, db:
                        db.execute(sql)
                    with self.assertRaisesRegex(Exception, message or "."):
                        self.open(
                            parameters={
                                "MetadataThreads": str(threads),
                                "MetadataBatchSize": str(batch),
                            }
                        )
                    # A failed Open must not leave a database handle or worker.
                    with closing(sqlite3.connect(self.index)) as db, db:
                        db.execute("CREATE TABLE after_failure(x)")

    def test_invalid_metadata_parameters(self):
        self.metadata_fixture()
        for key in ("MetadataThreads", "MetadataBatchSize"):
            for value in ("0", "-1", "2x", "", "999999999999999999999999999999"):
                with self.subTest(key=key, value=value), self.assertRaises(Exception):
                    self.open(parameters={key: value})

    def dataset_fixture(self, hdf5=False, series=False, leading_slash=False, multiblock=False):
        prefix = "bbb/" if leading_slash else ""
        source_prefix = "/" + prefix if leading_slash else ""
        files = ["part0.h5", "part1.h5"] if hdf5 else ["part0.bp", "part1.bp"]
        if not series:
            files = files[:1]
        for i, filename in enumerate(files):
            adios = adios2.Adios()
            io = adios.declare_io("writer")
            io.set_engine("HDF5" if hdf5 else "BP5")
            values = np.arange(6, dtype=np.float64)
            var = io.define_variable(source_prefix + "values", values, [6], [0], [6])
            scalar = io.define_variable(source_prefix + "scalar", np.zeros(1, np.int32))
            try:
                writer = io.open(str(self.root / filename), adios2.Mode.Write)
            except ValueError as error:
                if hdf5 and "HDF5" in str(error) and "compiled" in str(error):
                    self.skipTest("HDF5 unavailable")
                raise
            for step in range(2):
                writer.begin_step()
                if multiblock:
                    var.set_selection([[0], [2]])
                    writer.put(var, values[:2] + (2 * i + step) * 10, adios2.Mode.Sync)
                    var.set_selection([[2], [4]])
                    writer.put(var, values[2:] + (2 * i + step) * 10, adios2.Mode.Sync)
                else:
                    writer.put(var, values + (2 * i + step) * 10, adios2.Mode.Sync)
                writer.put(scalar, np.array([100 + 2 * i + step], np.int32), adios2.Mode.Sync)
                writer.end_step()
            writer.close()
        logical = "series" if series else files[0]
        fmt = "HDF5" if hdf5 else "ADIOS"
        self.name = "store/data.aca/" + logical + "/" + prefix + "values"
        with closing(sqlite3.connect(self.index)) as db, db:
            db.executescript(INDEX_SCHEMA)
            db.execute("INSERT INTO variabletypes VALUES (3,'double'),(4,'int32_t')")
            db.execute("INSERT INTO datasettypes VALUES (5,?)", (fmt,))
            db.execute("INSERT INTO datasets VALUES (1,1,13,?,5)", (logical,))
            db.execute(
                "INSERT INTO variables VALUES (1,1,?,3,?,NULL,NULL,1,'6',NULL)",
                (prefix + "values", 2 * len(files)),
            )
            db.execute(
                "INSERT INTO variables VALUES (1,1,?,4,?,NULL,NULL,0,'',NULL)",
                (prefix + "scalar", 2 * len(files)),
            )
        with closing(sqlite3.connect(self.archive)) as db, db:
            db.executescript(SCHEMA)
            db.execute("INSERT INTO info VALUES ('ACA','test','0.8',0)")
            db.execute("INSERT INTO host VALUES ('fixture-local','localhost',0,0,'')")
            db.execute("INSERT INTO directory VALUES (1,?,0,0)", (str(self.root),))
            if series:
                db.execute("INSERT INTO timeseries VALUES (7,'series')")
            for i, filename in enumerate(files):
                db.execute(
                    "INSERT INTO dataset(rowid,name,uuid,modtime,deltime,fileformat,tsid,tsorder) "
                    "VALUES (?,?,?,0,0,?,?,?)",
                    (13 + i, filename, "uuid-" + str(i), fmt, 7 if series else 0, i * 3),
                )
                db.execute("INSERT INTO replica VALUES (?,1,1,0,?,0,0,0,0)", (13 + i, filename))

    def test_multiblock_metadata_and_selection(self):
        self.dataset_fixture(series=True, leading_slash=True, multiblock=True)
        adios, io, engine = self.open()
        try:
            var = io.inquire_variable(self.name)
            for step in (0, 1, 2, 3, 0):
                blocks = engine.blocks_info(self.name, step)
                self.assertEqual(len(blocks), 2)
                self.assertEqual([b["Start"] for b in blocks], ["0", "2"])
                self.assertEqual([b["Count"] for b in blocks], ["2", "4"])
                self.assertEqual(float(blocks[1]["Min"]), step * 10 + 2)
                self.assertEqual(float(blocks[1]["Max"]), step * 10 + 5)
                var.set_step_selection([step, 1])
                var.set_block_selection(1)
                self.assertEqual(var.start(), [2])
                self.assertEqual(var.count(), [4])
                self.assertEqual(var.selection_size(), 4)
                data = np.zeros(4)
                engine.get(var, data, adios2.Mode.Sync)
                np.testing.assert_array_equal(data, np.arange(2, 6) + step * 10)
            with self.assertRaises(ValueError):
                engine.blocks_info(self.name, 4)
        finally:
            engine.close()

    def test_streaming_block_metadata(self):
        self.dataset_fixture(series=True, multiblock=True)
        adios, io, engine = self.open(adios2.Mode.Read)
        try:
            for step in range(4):
                self.assertEqual(engine.begin_step(), adios2.StepStatus.OK)
                blocks = engine.blocks_info(self.name, 0)
                self.assertEqual(float(blocks[1]["Min"]), step * 10 + 2)
                var = io.inquire_variable(self.name)
                var.set_block_selection(1)
                self.assertEqual(var.start(), [2])
                self.assertEqual(var.count(), [4])
                self.assertEqual(var.selection_size(), 4)
                engine.end_step()
        finally:
            engine.close()

    def test_metadata_is_lazy_and_missing_source_is_an_error(self):
        self.dataset_fixture()
        hidden = self.archive.with_suffix(".hidden")
        self.archive.rename(hidden)
        adios, io, engine = self.open()
        try:
            var = io.inquire_variable(self.name)
            self.assertEqual(var.shape(), [6])
            self.assertEqual(var.count(), [6])
            self.assertEqual(var.selection_size(), 6)
            with self.assertRaises(RuntimeError):
                engine.blocks_info(self.name, 0)
            hidden.rename(self.archive)
            self.assertEqual(len(engine.blocks_info(self.name, 0)), 1)
        finally:
            engine.close()

    def test_hdf5_block_selection_metadata(self):
        self.dataset_fixture(hdf5=True)
        adios, io, engine = self.open()
        try:
            var = io.inquire_variable(self.name)
            var.set_step_selection([1, 1])
            var.set_block_selection(0)
            self.assertEqual(var.start(), [0])
            self.assertEqual(var.count(), [6])
            self.assertEqual(var.selection_size(), 6)
            self.assertEqual(len(engine.blocks_info(self.name, 1)), 1)
        finally:
            engine.close()

    def check_dataset(self, hdf5=False, series=False, leading_slash=False):
        self.dataset_fixture(hdf5, series, leading_slash)
        before = self.archive.read_bytes()
        adios, io, engine = self.open(parameters={"MetadataThreads": "3", "MetadataBatchSize": "1"})
        try:
            var = io.inquire_variable(self.name)
            var.set_selection([[1], [3]])
            var.set_step_selection([1, 2 if series else 1])
            data = np.zeros((2 if series else 1, 3), np.float64)
            engine.get(var, data, adios2.Mode.Sync)
            np.testing.assert_array_equal(data[0], np.arange(1, 4) + 10)
            if series:
                np.testing.assert_array_equal(data[1], np.arange(1, 4) + 20)
            # Repeated deferred requests must retain their individual selections.
            a, b = np.zeros(2), np.zeros(3)
            var.set_step_selection([0, 1])
            var.set_selection([[0], [2]])
            engine.get(var, a, adios2.Mode.Deferred)
            var.set_selection([[3], [3]])
            engine.get(var, b, adios2.Mode.Deferred)
            engine.perform_gets()
            np.testing.assert_array_equal(a, [0, 1])
            np.testing.assert_array_equal(b, [3, 4, 5])
            scalar = io.inquire_variable(self.name.rsplit("/", 1)[0] + "/scalar")
            scalar.set_step_selection([1, 2 if series else 1])
            scalars = np.zeros(2 if series else 1, np.int32)
            engine.get(scalar, scalars, adios2.Mode.Sync)
            np.testing.assert_array_equal(scalars, [101, 102] if series else [101])
            if not hdf5:
                var.set_block_selection(0)
                var.set_step_selection([1, 1])
                block = np.zeros(6)
                engine.get(var, block, adios2.Mode.Sync)
                np.testing.assert_array_equal(block, np.arange(6) + 10)
        finally:
            engine.close()
        self.assertEqual(self.archive.read_bytes(), before)
        # A fresh reader can use the same ADIOS after closing cached engines.
        io = adios.declare_io("streaming-index")
        io.set_parameter("hostname", "fixture-local")
        io.set_parameters({"MetadataThreads": "3", "MetadataBatchSize": "1"})
        engine = io.open(str(self.index), adios2.Mode.Read)
        try:
            for step in range(4 if series else 2):
                self.assertEqual(engine.begin_step(), adios2.StepStatus.OK)
                var = io.inquire_variable(self.name)
                var.set_selection([[0], [6]])
                data = np.zeros(6)
                engine.get(var, data, adios2.Mode.Deferred)
                engine.end_step()
                np.testing.assert_array_equal(data, np.arange(6) + step * 10)
            self.assertEqual(engine.begin_step(), adios2.StepStatus.EndOfStream)
        finally:
            engine.close()

    def test_cached_engine_and_close(self):
        self.dataset_fixture()
        adios, io, engine = self.open()
        var = io.inquire_variable(self.name)
        data = np.zeros(6)
        engine.get(var, data, adios2.Mode.Sync)
        source = self.root / "part0.bp"
        moved = self.root / "moved.bp"
        source.rename(moved)
        try:
            # A second open by path would fail; the cached engine still reads.
            var.set_step_selection([1, 1])
            engine.get(var, data, adios2.Mode.Deferred)
            engine.close()
            np.testing.assert_array_equal(data, np.arange(6) + 10)
        finally:
            moved.rename(source)

    def test_single_step_scalar_uses_dataset(self):
        self.dataset_fixture()
        # No scalar value is cached in the index. Even a single-step indexed
        # scalar must be read from the underlying file rather than defaulting to zero.
        with closing(sqlite3.connect(self.index)) as db, db:
            db.execute("UPDATE variables SET steps=1 WHERE name='scalar'")
        adios, io, engine = self.open()
        try:
            scalar = io.inquire_variable(self.name.rsplit("/", 1)[0] + "/scalar")
            data = np.zeros(1, np.int32)
            engine.get(scalar, data, adios2.Mode.Sync)
            np.testing.assert_array_equal(data, [100])
        finally:
            engine.close()

    def test_bp_leading_slash(self):
        self.check_dataset(leading_slash=True)

    def test_bp_timeseries_leading_slash(self):
        self.check_dataset(series=True, leading_slash=True)

    def test_missing_variable_diagnostic(self):
        self.dataset_fixture()
        with closing(sqlite3.connect(self.index)) as db, db:
            db.execute("UPDATE variables SET name='missing' WHERE name='scalar'")
        adios, io, engine = self.open()
        try:
            var = io.inquire_variable(self.name.rsplit("/", 1)[0] + "/missing")
            with self.assertRaisesRegex(RuntimeError, "variable not found with indexed type"):
                engine.get(var, np.zeros(1, np.int32), adios2.Mode.Sync)
        finally:
            engine.close()

    def test_bp_dataset(self):
        self.check_dataset()

    def test_bp_timeseries(self):
        self.check_dataset(series=True)

    def test_hdf5_dataset(self):
        self.check_dataset(hdf5=True)

    def test_hdf5_timeseries(self):
        self.check_dataset(hdf5=True, series=True)

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
