// js-hacky way to use enum
const client_events = {
    WHEEL_UP: "wheel_up",
    WHEEL_DOWN: "wheel_down",
    MODIFIERS: {
        LEFT_MOUSE: "mod_left_mouse",
    }
};

let connect = function() {
    document.getElementById("connectButton").disabled = true;
    let port = document.getElementById("portInput").value;
    let address = "ws://127.0.0.1:" + port.toString() + "/stream";
    let socket = new WebSocket(address);
    let factor = 0.5;

    socket.onerror = function(error) {
        console.error(error);
        document.getElementById("connectButton").disabled = false;
    }

    socket.onopen = function(event) {
        console.log(event);
        var self = this;
        console.log("Connection opened");

        this.onmessage = function(event) {
            console.log("Message received");
            // @TODO @FUTURE probably retrieve the base64 image alongside other information (FPS, latency, ...) inside a json
            // @TODO check that the received message contains an image before updating
            update_image(event.data);
        }

        let interaction_callback = function(event, modifiers = []) {
            let data = {
                client_event: event,
                client_event_mods: modifiers,
            };
            console.log(data);
            self.send(JSON.stringify(data));
        }
/*
        document.addEventListener("keydown", function onPress(event) {
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
        });
        */
        document.onwheel = function(event) {
            if (event.deltaY < 0) {
                interaction_callback(client_events.WHEEL_DOWN);
            } else {
                interaction_callback(client_events.WHEEL_UP);
            }
        }
    }

    socket.onclose = function(event) {
        document.getElementById("connectButton").disabled = false;
    }

    let update_image = function(image_data) {
        let image_elem = document.getElementById("frame");
        image_elem.src = "data:image/jpg;base64," + image_data;
    }

}

