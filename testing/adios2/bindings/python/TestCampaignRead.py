#!/usr/bin/env python
# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
# SPDX-License-Identifier: Apache-2.0
"""Reader compatibility with ACA 0.7 and the additive ACA 0.8 schema."""

import socket
import sqlite3
import tempfile
import unittest
from contextlib import closing
from pathlib import Path

import adios2
import numpy as np


SCHEMA = """
CREATE TABLE info(id TEXT, name TEXT, version TEXT, modtime INT);
CREATE TABLE key(keyid TEXT PRIMARY KEY);
CREATE TABLE host(hostname TEXT PRIMARY KEY, longhostname TEXT, modtime INT,
                  deltime INT, default_protocol TEXT);
CREATE TABLE directory(hostid INT, name TEXT, modtime INT, deltime INT,
                       PRIMARY KEY(hostid,name));
CREATE TABLE timeseries(tsid INTEGER PRIMARY KEY, name TEXT UNIQUE);
CREATE TABLE dataset(name TEXT PRIMARY KEY, uuid TEXT, modtime INT, deltime INT,
                     fileformat TEXT, tsid INT, tsorder INT);
CREATE TABLE replica(datasetid INT, hostid INT, dirid INT, archiveid INT, name TEXT,
                     modtime INT, deltime INT, keyid INT, size INT);
CREATE TABLE file(fileid INTEGER PRIMARY KEY, name TEXT, compression INT,
                  lenorig INT, lencompressed INT, modtime INT, checksum TEXT, data BLOB);
CREATE TABLE repfiles(replicaid INT, fileid INT, PRIMARY KEY(replicaid,fileid));
CREATE TABLE accuracy(replicaid INT PRIMARY KEY, accuracy REAL, norm REAL, relative INT);
CREATE TABLE resolution(replicaid INT PRIMARY KEY, x INT, y INT);
CREATE TABLE archive(dirid INT, tarname TEXT, system TEXT, notes BLOB);
CREATE TABLE archiveidx(archiveid INT, replicaid INT, filename TEXT,
                        offset INT, offset_data INT, size INT);
"""


class TestCampaignRead(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.values = np.arange(6, dtype=np.float64)
        with adios2.Stream(str(self.root / "data.bp"), "w") as writer:
            writer.write("values", self.values, [6], [0], [6])

    def campaign(self, version, sparse=False, deleted=False):
        path = self.root / "data.aca"
        host, directory, key = (17, 29, 9) if sparse else (1, 1, 1)
        with closing(sqlite3.connect(path)) as database, database:
            database.executescript(SCHEMA)
            database.execute("INSERT INTO info VALUES ('ACA','Campaign',?,0)", (version,))
            database.execute("INSERT INTO key(rowid,keyid) VALUES (?,'unused-local-key')", (key,))
            database.execute(
                "INSERT INTO host(rowid,hostname,longhostname,default_protocol) VALUES (?,?,?,'')",
                (host, socket.gethostname(), socket.getfqdn()),
            )
            database.execute(
                "INSERT INTO directory(rowid,hostid,name) VALUES (?,?,?)",
                (directory, host, str(self.root)),
            )
            database.execute(
                "INSERT INTO dataset(rowid,name,uuid,modtime,deltime,fileformat,tsid,tsorder) "
                "VALUES (13,'data.bp','dataset-uuid',0,0,'ADIOS',0,0)"
            )
            database.execute(
                "INSERT INTO replica VALUES (13,?,?,0,'data.bp',0,0,?,0)", (host, directory, key)
            )
            if deleted:
                database.execute(
                    "INSERT INTO dataset(rowid,name,uuid,modtime,deltime,fileformat,tsid,tsorder) "
                    "VALUES (31,'gone.bp','gone-uuid',0,1,'ADIOS',0,0)"
                )
                database.execute(
                    "INSERT INTO replica VALUES (31,?,?,0,'gone.bp',0,0,0,0)", (host, directory)
                )
            if version == "0.8":
                # Additive merge tables are intentionally outside the reader's queries.
                database.execute(
                    "CREATE TABLE entity_identity(kind TEXT, local_id INT, uuid TEXT, active INT)"
                )
                database.execute("CREATE TABLE change_log(uuid TEXT PRIMARY KEY, after_value TEXT)")
                database.execute("INSERT INTO change_log VALUES ('ignored','not application data')")
        return path

    def check_read(self, version, sparse=False, deleted=False):
        path = self.campaign(version, sparse, deleted)
        before = path.read_bytes()
        adios = adios2.Adios()
        io = adios.declare_io("campaign-test")
        io.set_parameter("hostname", socket.gethostname())
        with adios2.FileReader(io, str(path)) as reader:
            self.assertIn("data.bp/values", reader.available_variables())
            self.assertNotIn("gone.bp/values", reader.available_variables())
            np.testing.assert_array_equal(reader.read("data.bp/values"), self.values)
        self.assertEqual(path.read_bytes(), before)

    def test_07(self):
        self.check_read("0.7")

    def test_08(self):
        self.check_read("0.8")

    def test_sparse_07(self):
        self.check_read("0.7", sparse=True)

    def test_sparse_08(self):
        self.check_read("0.8", sparse=True)

    def test_deleted_adios_dataset(self):
        self.check_read("0.8", sparse=True, deleted=True)

    def test_unsupported_version(self):
        path = self.campaign("0.9")
        with self.assertRaisesRegex(ValueError, "reads ACA versions 0.7 and 0.8"):
            adios2.FileReader(str(path))


if __name__ == "__main__":
    unittest.main()
