async function updateDashboard() {
    const res = await fetch("/status");
    const data = await res.json();

    const connected = data.connected;
    const robots = data.robots;

    // Arduino connection status
    document.getElementById("arduino-status").textContent =
        connected ? "Arduino connected" : "Arduino not connected";

    // Update each robot card
    for (const [name, state] of Object.entries(robots)) {
        const card = document.getElementById(name);
        card.querySelector(".left_wheel").textContent = state.left_wheel ?? "--";
        card.querySelector(".right_wheel").textContent = state.right_wheel ?? "--";
        card.querySelector(".distance").textContent = state.distance ?? "--";
        card.querySelector(".gripper").textContent = state.gripper ?? "--";
        card.querySelector(".time").textContent = state.time ?? "--";
    }
}

// Refresh every second
setInterval(updateDashboard, 1000);
updateDashboard();

document.getElementById("show-image-btn").addEventListener("click", () => {
    const container = document.getElementById("image-container");

    // If image already exists, do nothing
    if (!container.querySelector("img")) {
        const img = document.createElement("img");
        img.src = "/static/image.png"; // path to your image
        img.alt = "Displayed Image";
        img.style.maxWidth = "100%";    // responsive width
        container.appendChild(img);
    }
});

