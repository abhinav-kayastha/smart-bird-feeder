const sqlite3 = require("sqlite3").verbose();
const db = new sqlite3.Database("C:\\Users\\abhin\\test.db"); // Use a file-based database

module.exports = db;
