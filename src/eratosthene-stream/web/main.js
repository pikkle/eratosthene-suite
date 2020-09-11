// js-hacky way to use enum
const request_events = {
    CANVAS_SIZE: "canvas_size",
    SET_VIEW: "set_view",
    CLIENT_EVENT: "client_event",
}
const client_events = {
    WHEEL_UP: "wheel_up",
    WHEEL_DOWN: "wheel_down",
    LEFT_MOUSE_MOVEMENT: "left_mouse_move",
    RIGHT_MOUSE_MOVEMENT: "right_mouse_move",
    BUTTON_A: "button_a",
    BUTTON_S: "button_s",
    MODIFIERS: {
        LEFT_MOUSE: "mod_left_mouse",
    },
};

let DEFAULT_LON = 6.126579;
let DEFAULT_LAT = 46.2050282;
let DEFAULT_ALT = 6441918.37;
let DEFAULT_TIA = 1117584000;
let DEFAULT_TIB = 1117584000;

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

document.getElementById("reset_button").disabled = true;

let connect = function() {
    document.getElementById("connect_button").disabled = true;
    // @TODO: change ip+port of streaming server to the actual server address
    let port = document.getElementById("port_input").value;
    let address = "ws://127.0.0.1:" + port.toString() + "/stream";
    let socket = new WebSocket(address);

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
        // reanable the connect button if the connection gets close
        document.getElementById("connect_button").disabled = false;
    }

    socket.onopen = function(event) {
        var self = this;
        console.log("Connection opened");

        // Initialize screen size corresponding to the canvas size available
        self.send(JSON.stringify({
            request: request_events.CANVAS_SIZE,
            data: {
                width: canvas.width,
                height: canvas.height,
            }
        }));

        let set_viewpoint = function(lat = DEFAULT_LAT, lon = DEFAULT_LON,
                                     alt = DEFAULT_ALT,
                                     tia = DEFAULT_TIA, tib = DEFAULT_TIB) {
            self.send(JSON.stringify({
                request: request_events.SET_VIEW,
                data: {
                    view_lat: lat,
                    view_lon: lon,
                    view_alt: alt,
                    view_tia: tia,
                    view_tib: tib,
                }
            }));
        }

        // setup reset button to reset the position at start
        document.getElementById("reset_button").disabled = false;
        document.getElementById("reset_button").onclick = function() {
            set_viewpoint();
        };

        // handle received messages
        this.onmessage = function(event) {
            let json = JSON.parse(event.data);
            if (json.hasOwnProperty("frame")) {
                update_image(json.frame);
            }
            if (json.hasOwnProperty("view")) {
                update_view_info(json.view);
            }
        }

        // callback to send inputs to the streaming server
        let interaction_callback = function(event = "", modifiers = [], data = {}) {
            let payload = {
                request: request_events.CLIENT_EVENT,
                data: data,
                client_event: event,
                client_event_mods: modifiers,
            };
            self.send(JSON.stringify(payload));
        }

        // keyboard registration
        document.onkeydown = function(event) {
            switch (event.key) {
                case "a":
                    interaction_callback(client_events.BUTTON_A); break;
                case "s":
                    interaction_callback(client_events.BUTTON_S); break;
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
        document.getElementById("connect_button").disabled = false;
    }

    // display received image frames
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

    // update additional information about the view point
    let update_view_info = function(view_data) {
        document.getElementById("data_lat").textContent = view_data.lat;
        document.getElementById("data_lon").textContent = view_data.lon;
        document.getElementById("data_alt").textContent = view_data.alt;
        document.getElementById("data_tia").textContent = view_data.tia;
        document.getElementById("data_tib").textContent = view_data.tib;
        document.getElementById("data_azm").textContent = view_data.azm;
        document.getElementById("data_gam").textContent = view_data.gam;
        document.getElementById("data_spn").textContent = view_data.spn;
    }

}

