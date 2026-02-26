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

document.getElementById("show-video-btn").addEventListener("click", function () {

    const video = document.getElementById("video-container");

    if (video.style.display === "none") {
        video.style.display = "block";
    } else {
        video.style.display = "none";
    }

});
