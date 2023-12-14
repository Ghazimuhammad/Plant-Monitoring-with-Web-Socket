var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function toggleWatering() {
    var wateringSwitch = document.getElementById("wateringSwitch");
    websocket.send("toggleWatering:" + wateringSwitch.checked);
}

function setMoistureThreshold() {
    var thresholdInput = document.getElementById("threshold");
    var thresholdValue = thresholdInput.value;

    alert("Moisture Threshold set to: " + thresholdValue + "%");

    websocket.send("setMoistureThreshold:" + thresholdValue);
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}


function onOpen(event) {
    console.log('Connection opened');
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
    console.log(event.data);
    var myObj = JSON.parse(event.data);

    document.getElementById('temperature').innerHTML = myObj.temperature || 'N/A';
    document.getElementById('humidity').innerHTML = myObj.humidity || 'N/A';
    document.getElementById('moisture').innerHTML = myObj.moisture || 'N/A';

    var wateringSwitch = document.getElementById("wateringSwitch");
    wateringSwitch.checked = myObj.watering || false;
}