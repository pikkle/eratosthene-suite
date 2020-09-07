// js-hacky way to use enum
// @TODO @OPTIM to handle lighter messages, this could be changed to a single int with a flag system

const request_events = {
    CANVAS_SIZE: "canvas_size",
    CLIENT_EVENT: "client_event",
}
const client_events = {
    WHEEL_UP: "wheel_up",
    WHEEL_DOWN: "wheel_down",
    LEFT_MOUSE_MOVEMENT: "left_mouse_move",
    RIGHT_MOUSE_MOVEMENT: "right_mouse_move",
    MODIFIERS: {
        LEFT_MOUSE: "mod_left_mouse",
    },
};

document.pointerLockElement = document.pointerLockElement    ||
    document.mozPointerLockElement ||
    document.webkitPointerLockElement;

let pointer_locked = false;
function pointerLockChange() {
    pointer_locked = !!document.pointerLockElement;
}

document.addEventListener('pointerlockchange', pointerLockChange, false);
document.addEventListener('mozpointerlockchange', pointerLockChange, false);
document.addEventListener('webkitpointerlockchange', pointerLockChange, false);

let canvas = document.getElementById("canvas_stream");
let canvas_container = document.getElementById("canvas_container");
canvas.width = canvas_container.clientWidth;
canvas.height = canvas_container.clientHeight;
console.log("Canvas size " + canvas.width + "x" + canvas.height);


let connect = function() {
    document.getElementById("connect_button").disabled = true;
    let port = document.getElementById("port_input").value;
    let address = "ws://127.0.0.1:" + port.toString() + "/stream";
    let socket = new WebSocket(address);
    let factor = 0.5;


    // compatibility to request pointer lock on canvas
    // https://developer.mozilla.org/fr/docs/WebAPI/Pointer_Lock
    canvas.requestPointerLock = canvas.requestPointerLock ||
        canvas.mozRequestPointerLock ||
        canvas.webkitPointerLockElement;

    // capture mouse in canvas
    canvas.onclick = function(event) {
        if (!pointer_locked) {
            canvas.requestPointerLock();
        }
    }

    socket.onerror = function(error) {
        console.error(error);
        document.getElementById("connectButton").disabled = false;
    }

    socket.onopen = function(event) {
        console.log(event);
        var self = this;
        console.log("Connection opened");

        socket.send(JSON.stringify({
                request: request_events.CANVAS_SIZE,
                width: canvas.width,
                height: canvas.height,
        }));

        this.onmessage = function(event) {
            // @TODO @FUTURE probably retrieve the base64 image alongside other information (FPS, latency, ...) inside a json
            // @TODO check that the received message contains an image before updating
            update_image(event.data);
        }

        // callback to send inputs to the streaming server
        let interaction_callback = function(event, modifiers = [], data = {}) {
            let payload = {
                request: request_events.CLIENT_EVENT,
                client_event: event,
                client_event_mods: modifiers,
                client_event_data: data,
            };
            self.send(JSON.stringify(payload));
        }

        // keyboard registration
        document.onkeydown = function(event) {
            switch (event.key) {
                /*
                case "ArrowLeft":
                    transform.rotate_z = +factor; break;
                */
            }
        }

        // mouse wheel registration
        document.onwheel = function(event) {
            if (pointer_locked) {
                if (event.deltaY < 0) {
                    interaction_callback(client_events.WHEEL_DOWN);
                } else {
                    interaction_callback(client_events.WHEEL_UP);
                }
            }
        }

        // mouse movements registration
        document.onmousemove = function(event) {
            if (pointer_locked) {
                let movement_data = {
                    "dx" : event.movementX,
                    "dy" : event.movementY,
                };
                switch (event.buttons) {
                    case 1: // left click drag
                        interaction_callback(client_events.LEFT_MOUSE_MOVEMENT, [], movement_data);
                        break;
                    case 2: // right click drag
                        interaction_callback(client_events.RIGHT_MOUSE_MOVEMENT, [], movement_data);
                        break;
                }
            }
        }
    }

    socket.onclose = function(event) {
        document.getElementById("connectButton").disabled = false;
    }

    let update_image = function(image_data) {
        var i = new Image();
        i.onload = function(){
            canvas.width = i.width;
            canvas.height = i.height;
        };
        i.src = "data:image/jpg;base64," + image_data;
        canvas.style.background = "url(" + i.src + ")";
        let image_elem = document.getElementById("frame");
    }

}

