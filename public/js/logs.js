let currentPage = 1;
const itemsPerPage = 50;
let sensorData = [];

async function fetchSensorData() {
  try {
<<<<<<< HEAD
    const response = await fetch('/sensor-data');
=======
    const response = await fetch("/sensor-data");
>>>>>>> 838d6afec3f08a15fc18c5f5f5604327d0f953da
    const data = await response.json();
    sensorData = data.data; // No need to reverse as the latest entries are already first
    displayPage(currentPage);
  } catch (error) {
<<<<<<< HEAD
    console.error('Error fetching sensor data:', error);
    document.getElementById('Database Data').innerText = 'Error loading sensor data';
=======
    console.error("Error fetching sensor data:", error);
    document.getElementById("Database Data").innerText =
      "Error loading sensor data";
>>>>>>> 838d6afec3f08a15fc18c5f5f5604327d0f953da
  }
}

function displayPage(page) {
<<<<<<< HEAD
  const sensorDataList = document.getElementById('Database Data');
  sensorDataList.innerHTML = '';
=======
  const sensorDataList = document.getElementById("Database Data");
  sensorDataList.innerHTML = "";
>>>>>>> 838d6afec3f08a15fc18c5f5f5604327d0f953da

  const start = (page - 1) * itemsPerPage;
  const end = start + itemsPerPage;
  const pageData = sensorData.slice(start, end);

<<<<<<< HEAD
  pageData.forEach(item => {
    const listItem = document.createElement('li');
=======
  pageData.forEach((item) => {
    const listItem = document.createElement("li");
>>>>>>> 838d6afec3f08a15fc18c5f5f5604327d0f953da
    listItem.textContent = `Time: ${item.timestamp}, Status: ${item.status}`;
    sensorDataList.appendChild(listItem);
  });

<<<<<<< HEAD
  document.getElementById('pageInfo').textContent = `Page ${page} of ${Math.ceil(sensorData.length / itemsPerPage)}`;
  document.getElementById('prevPage').disabled = page === 1;
  document.getElementById('nextPage').disabled = end >= sensorData.length;
=======
  document.getElementById(
    "pageInfo"
  ).textContent = `Page ${page} of ${Math.ceil(
    sensorData.length / itemsPerPage
  )}`;
  document.getElementById("prevPage").disabled = page === 1;
  document.getElementById("nextPage").disabled = end >= sensorData.length;
>>>>>>> 838d6afec3f08a15fc18c5f5f5604327d0f953da
}

function prevPage() {
  if (currentPage > 1) {
    currentPage--;
    displayPage(currentPage);
  }
}

function nextPage() {
  if (currentPage * itemsPerPage < sensorData.length) {
    currentPage++;
    displayPage(currentPage);
  }
}

function downloadLogs() {
<<<<<<< HEAD
  const csvContent = "data:text/csv;charset=utf-8," 
    + sensorData.map(e => `${e.timestamp},${e.status}`).join("\n");
=======
  const csvContent =
    "data:text/csv;charset=utf-8," +
    sensorData.map((e) => `${e.timestamp},${e.status}`).join("\n");
>>>>>>> 838d6afec3f08a15fc18c5f5f5604327d0f953da

  const encodedUri = encodeURI(csvContent);
  const link = document.createElement("a");
  link.setAttribute("href", encodedUri);
  link.setAttribute("download", "logs.csv");
  document.body.appendChild(link);

  link.click();
  document.body.removeChild(link);
}

// Fetch sensor data on page load
window.onload = fetchSensorData;

// WebSocket connection to receive real-time updates
<<<<<<< HEAD
const socket = new WebSocket('ws://' + window.location.host);

socket.onmessage = function(event) {
  const message = JSON.parse(event.data);
  if (message.type === 'new-data') {
    fetchSensorData();
  }
};
=======
const socket = new WebSocket("ws://" + window.location.host);

socket.onmessage = function (event) {
  const message = JSON.parse(event.data);
  if (message.type === "new-data") {
    fetchSensorData();
  }
};
>>>>>>> 838d6afec3f08a15fc18c5f5f5604327d0f953da
