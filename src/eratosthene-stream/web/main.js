let socket = new WebSocket("ws://127.0.0.1:8080/stream");
socket.onerror = function(error) {
    console.error(error);
}

socket.onopen = function(event) {
    console.log(event);
    var s = this;
    console.log("Connection opened");

    this.onclose = function(event) {
        console.log("Connection closed");
    }

    this.onmessage = function(event) {
        console.log("Message received");
        // @FUTURE probably retrieve the base64 image alongside other information (FPS, latency, ...) inside a json
        update_image(event.data);
        // this.close();
    }
    document.addEventListener("keydown", function onPress(event) {
        let transform = {
            rotate_x : 0,
            rotate_y : 0,
            rotate_z : 0,
        };
        switch (event.key) {
            case "ArrowLeft":
                transform.rotate_z = +2;
                s.send(JSON.stringify(transform));
                break;
            case "ArrowRight":
                transform.rotate_z = -2;
                s.send(JSON.stringify(transform));
                break;
            case "ArrowUp":
                transform.rotate_x = -2;
                s.send(JSON.stringify(transform));
                break;
            case "ArrowDown":
                transform.rotate_x = +2;
                s.send(JSON.stringify(transform));
                break;
        }
    });
}

let update_image = function(image_data) {
    let image_elem = document.getElementById("frame");
    image_elem.src = "data:image/jpg;base64," + image_data;
}