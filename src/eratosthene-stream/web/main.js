let socket = new WebSocket("ws://127.0.0.1:8080/stream");
socket.onerror = function(error) {
    console.error(error);
}

socket.onopen = function(event) {
    console.log("Connection opened");

    this.onclose = function(event) {
        console.log("Connection closed");
    }

    this.onmessage = function(event) {
        console.log("Message received");
        console.log(event.data);
        // @FUTURE probably retrieve the base64 image alongside other information (FPS, latency, ...) inside a json
        update_image(event.data);
    }

    this.send("Hello world");
}

let update_image = function(image_data) {
    let image_elem = document.getElementById("frame");
    image_elem.src = "data:image/jpg;base64," + image_data;
}