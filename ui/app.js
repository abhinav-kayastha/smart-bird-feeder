const express = require("express");
const path = require("path");
const app = express();

app.get("/", (req, res) => {
  res.sendFile(path.join(__dirname, "index.html"));
});

app.get("/activity", (req, res) => {
  res.sendFile(path.join(__dirname, "activity.html"));
});

app.listen(3000, () => {
  console.log("Server is running on port 3000");
});
