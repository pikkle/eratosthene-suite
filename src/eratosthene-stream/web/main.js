let connect = function() {
    document.getElementById("connectButton").disabled = true;
    let port = document.getElementById("portInput").value;
    let address = "ws://127.0.0.1:" + port.toString() + "/stream";
    let socket = new WebSocket(address);
    // if (socket.CLOSED) {
    //     document.getElementById("connectButton").disabled = false;
    // }
    let factor = 0.5;

    socket.onerror = function(error) {
        console.error(error);
        document.getElementById("connectButton").disabled = false;
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
                translate_camera_x : 0,
                translate_camera_y : 0,
                translate_camera_z : 0,
                zoom : 0,
            };
            switch (event.key) {
                case "ArrowLeft":
                    transform.rotate_z = +factor; break;
                case "ArrowRight":
                    transform.rotate_z = -factor; break;
                case "ArrowUp":
                    transform.zoom = +factor; break;
                case "ArrowDown":
                    transform.zoom = -factor; break;
            }
            console.log("Sending object :");
            console.log(transform);
            s.send(JSON.stringify(transform));
        });
    }

    socket.onclose = function(event) {
        document.getElementById("connectButton").disabled = false;
    }

    let update_image = function(image_data) {
        let image_elem = document.getElementById("frame");
        image_elem.src = "data:image/jpg;base64," + image_data;
    }
}

