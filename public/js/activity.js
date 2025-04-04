let currentGraph = 0;
const graphs = ["hourlyChart", "weeklyChart", "monthlyChart", "yearlyChart"];
const graphTitles = [
  "Hourly Activity Data",
  "Weekly Activity Data",
  "Daily Activity Data for Current Month",
  "Monthly Activity Data",
];

let hourlyChart, weeklyChart, monthlyChart, yearlyChart;
let hoursUnlocked = Array(24).fill(0);
let hoursLocked = Array(24).fill(0);
let daysUnlocked = Array(7).fill(0);
let daysLocked = Array(7).fill(0);
let dailyUnlocked = Array(31).fill(0); // Assuming a maximum of 31 days in a month
let dailyLocked = Array(31).fill(0);
let monthsUnlocked = Array(12).fill(0);
let monthsLocked = Array(12).fill(0);
async function fetchActivityData() {
  try {
    const response = await fetch("/sensor-data");
    const data = await response.json();
    const activityData = data.data;
    // Process data to get the number of entries per hour, day, month, and day of the current month
    hoursUnlocked = Array(24).fill(0);
    hoursLocked = Array(24).fill(0);
    daysUnlocked = Array(7).fill(0);
    daysLocked = Array(7).fill(0);
    dailyUnlocked = Array(31).fill(0);
    dailyLocked = Array(31).fill(0);
    monthsUnlocked = Array(12).fill(0);
    monthsLocked = Array(12).fill(0);
    activityData.forEach((item) => {
      const [date, time] = item.timestamp.split(", ");
      const [day, month, year] = date.split("/");
      const hour = parseInt(time.split(":")[0], 10);
      const dayOfWeek = new Date(`${year}-${month}-${day}`).getDay();
      const dayOfMonth = parseInt(day, 10) - 1;
      const monthIndex = parseInt(month, 10) - 1;
      if (item.status === "Unlocked") {
        // Update unlocked data
        hoursUnlocked[hour]++;
        daysUnlocked[dayOfWeek]++;
        if (
          parseInt(month, 10) === new Date().getMonth() + 1 &&
          parseInt(year, 10) === new Date().getFullYear()
        ) {
          dailyUnlocked[dayOfMonth]++;
        }
        monthsUnlocked[monthIndex]++;
      } else if (item.status === "Locked") {
        // Update locked data
        hoursLocked[hour]++;
        daysLocked[dayOfWeek]++;
        if (
          parseInt(month, 10) === new Date().getMonth() + 1 &&
          parseInt(year, 10) === new Date().getFullYear()
        ) {
          dailyLocked[dayOfMonth]++;
        }
        monthsLocked[monthIndex]++;
      }
    });
    // Create the charts
    createCharts();
  } catch (error) {
    console.error("Error fetching activity data:", error);
  }
}
function createCharts() {
  const hourlyCtx = document.getElementById("hourlyChart").getContext("2d");
  hourlyChart = new Chart(hourlyCtx, {
    type: "bar",
    data: {
      labels: Array.from({ length: 24 }, (_, i) => `${i}`),
      datasets: [
        {
          label: "Unlocked",
          data: hoursUnlocked,
          backgroundColor: "rgba(75, 192, 192, 0.8)", // Increased opacity
          borderColor: "rgba(75, 192, 192, 1)",
          borderWidth: 1,
        },
        {
          label: "Locked",
          data: hoursLocked,
          backgroundColor: "rgba(255, 99, 132, 0.8)", // Increased opacity
          borderColor: "rgba(255, 99, 132, 1)",
          borderWidth: 1,
        },
      ],
    },
    options: {
      scales: {
        x: {
          beginAtZero: true,
        },
        y: {
          beginAtZero: true,
        },
      },
    },
  });
  const weeklyCtx = document.getElementById("weeklyChart").getContext("2d");
  weeklyChart = new Chart(weeklyCtx, {
    type: "bar",
    data: {
      labels: [
        "Sunday",
        "Monday",
        "Tuesday",
        "Wednesday",
        "Thursday",
        "Friday",
        "Saturday",
      ],
      datasets: [
        {
          label: "Unlocked",
          data: daysUnlocked,
          backgroundColor: "rgba(75, 192, 192, 0.8)", // Increased opacity
          borderColor: "rgba(75, 192, 192, 1)",
          borderWidth: 1,
        },
        {
          label: "Locked",
          data: daysLocked,
          backgroundColor: "rgba(255, 99, 132, 0.8)", // Increased opacity
          borderColor: "rgba(255, 99, 132, 1)",
          borderWidth: 1,
        },
      ],
    },
    options: {
      scales: {
        x: {
          beginAtZero: true,
        },
        y: {
          beginAtZero: true,
        },
      },
    },
  });
  const monthlyCtx = document.getElementById("monthlyChart").getContext("2d");
  monthlyChart = new Chart(monthlyCtx, {
    type: "bar",
    data: {
      labels: Array.from({ length: 31 }, (_, i) => `${i + 1}`),
      datasets: [
        {
          label: "Unlocked",
          data: dailyUnlocked,
          backgroundColor: "rgba(75, 192, 192, 0.8)", // Increased opacity
          borderColor: "rgba(75, 192, 192, 1)",
          borderWidth: 1,
        },
        {
          label: "Locked",
          data: dailyLocked,
          backgroundColor: "rgba(255, 99, 132, 0.8)", // Increased opacity
          borderColor: "rgba(255, 99, 132, 1)",
          borderWidth: 1,
        },
      ],
    },
    options: {
      scales: {
        x: {
          beginAtZero: true,
        },
        y: {
          beginAtZero: true,
        },
      },
    },
  });
  const yearlyCtx = document.getElementById("yearlyChart").getContext("2d");
  yearlyChart = new Chart(yearlyCtx, {
    type: "bar",
    data: {
      labels: [
        "January",
        "February",
        "March",
        "April",
        "May",
        "June",
        "July",
        "August",
        "September",
        "October",
        "November",
        "December",
      ],
      datasets: [
        {
          label: "Unlocked",
          data: monthsUnlocked,
          backgroundColor: "rgba(75, 192, 192, 0.8)", // Increased opacity
          borderColor: "rgba(75, 192, 192, 1)",
          borderWidth: 1,
        },
        {
          label: "Locked",
          data: monthsLocked,
          backgroundColor: "rgba(255, 99, 132, 0.8)", // Increased opacity
          borderColor: "rgba(255, 99, 132, 1)",
          borderWidth: 1,
        },
      ],
    },
    options: {
      scales: {
        x: {
          beginAtZero: true,
        },
        y: {
          beginAtZero: true,
        },
      },
    },
  });
  updateGraphInfo();
}
function updateCharts() {
  fetch("/sensor-data")
    .then((response) => response.json())
    .then((data) => {
      const activityData = data.data;
      // Process data to get the number of entries per hour, day, month, and day of the current month
      hoursUnlocked = Array(24).fill(0);
      hoursLocked = Array(24).fill(0);
      daysUnlocked = Array(7).fill(0);
      daysLocked = Array(7).fill(0);
      dailyUnlocked = Array(31).fill(0);
      dailyLocked = Array(31).fill(0);
      monthsUnlocked = Array(12).fill(0);
      monthsLocked = Array(12).fill(0);
      activityData.forEach((item) => {
        const [date, time] = item.timestamp.split(", ");
        const [day, month, year] = date.split("/");
        const hour = parseInt(time.split(":")[0], 10);
        const dayOfWeek = new Date(`${year}-${month}-${day}`).getDay();
        const dayOfMonth = parseInt(day, 10) - 1;
        const monthIndex = parseInt(month, 10) - 1;
        if (item.status === "Unlocked") {
          // Update unlocked data
          hoursUnlocked[hour]++;
          daysUnlocked[dayOfWeek]++;
          if (
            parseInt(month, 10) === new Date().getMonth() + 1 &&
            parseInt(year, 10) === new Date().getFullYear()
          ) {
            dailyUnlocked[dayOfMonth]++;
          }
          monthsUnlocked[monthIndex]++;
        } else if (item.status === "Locked") {
          // Update locked data
          hoursLocked[hour]++;
          daysLocked[dayOfWeek]++;
          if (
            parseInt(month, 10) === new Date().getMonth() + 1 &&
            parseInt(year, 10) === new Date().getFullYear()
          ) {
            dailyLocked[dayOfMonth]++;
          }
          monthsLocked[monthIndex]++;
        }
      });
      // Update the chart data
      hourlyChart.data.datasets[0].data = hoursUnlocked;
      hourlyChart.data.datasets[1].data = hoursLocked;
      hourlyChart.update();
      weeklyChart.data.datasets[0].data = daysUnlocked;
      weeklyChart.data.datasets[1].data = daysLocked;
      weeklyChart.update();
      monthlyChart.data.datasets[0].data = dailyUnlocked;
      monthlyChart.data.datasets[1].data = dailyLocked;
      monthlyChart.update();
      yearlyChart.data.datasets[0].data = monthsUnlocked;
      yearlyChart.data.datasets[1].data = monthsLocked;
      yearlyChart.update();
    })
    .catch((error) => console.error("Error updating charts:", error));
}
function updateGraphInfo() {
  document.getElementById("graphTitle").textContent = graphTitles[currentGraph];
  document.getElementById("prevGraph").textContent = `Previous (${
    graphTitles[(currentGraph - 1 + graphs.length) % graphs.length]
  })`;
  document.getElementById("nextGraph").textContent = `Next (${
    graphTitles[(currentGraph + 1) % graphs.length]
  })`;
}
function showGraph(index) {
  graphs.forEach((graph, i) => {
    document.getElementById(graph).style.display =
      i === index ? "block" : "none";
  });
  currentGraph = index;
  updateGraphInfo();
}
function prevGraph() {
  showGraph((currentGraph - 1 + graphs.length) % graphs.length);
}
function nextGraph() {
  showGraph((currentGraph + 1) % graphs.length);
}
// Fetch activity data on page load
window.onload = fetchActivityData;
// WebSocket connection to receive real-time updates
const socket = new WebSocket("ws://" + window.location.host);
socket.onmessage = function (event) {
  const message = JSON.parse(event.data);
  if (message.type === "new-data") {
    updateCharts();
  }
};
