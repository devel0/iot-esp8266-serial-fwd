// websocket client
var connection = new WebSocket('ws://' + location.hostname + ':81/', ['arduino']);

// retrieve html elements
var rxDataDiv = document.getElementById("rxDataDiv");
var serSpeedTxt = document.getElementById("serSpeed");
var serDataSendTxt = document.getElementById("serDataSend");
var autoUppercaseCB = document.getElementById("autoUpperCaseCB");

// ws on open ( send back some test data )
connection.onopen = function () {
    connection.send('Connect ' + new Date());
    console.log('ws opened');
};

// ws on error ( display an alert )
connection.onerror = function (error) {
    alert('ws error: ' + error);
};

var scrollSet = false;

// ws on message ( server send serial data readen back through websocket, display then auto scroll to bottom )
connection.onmessage = function (e) {
    if (e.data instanceof Blob) {
        e.data.arrayBuffer().then(buffer => {
            const view = new DataView(buffer);
            let len = view.byteLength;

            console.log('ws msg len=' + len);

            var str = "";
            let last_was_crlf = false;

            for (i = 0; i < len; ++i) {
                let c = view.getUint8(i);

                if (!last_was_crlf && (c == 10 || c == 13))
                {
                    last_was_crlf = true;
                    str += "<br/>";
                }
                else
                {
                    last_was_crlf = false;
                    str += String.fromCharCode(c);
                }
            }

            rxDataDiv.innerHTML += str;

            if (!scrollSet) {
                scrollSet = true;
                window.setTimeout(function () {
                    rxDataDiv.scrollTop = rxDataDiv.scrollHeight;
                    scrollSet = false;
                }, 200);
            }
        });
    }
};

// ws close
connection.onclose = function () {
    console.log('ws conn closed');
    alert("conn closed");
};

// change serial speed, saving config through /serParam api (GET)
async function reconnectClick() {
    const resp = await fetch("/serParam?" + encodeURIComponent('speed') + '=' + encodeURIComponent(serSpeedTxt.value), {
        method: 'GET',
        headers: {
            'Content-Type': 'application/x-www-form-urlencoded'
        }
    });
    if (resp.status == "200") {
        alert('data saved');
    }
}

// send serial data from serDataSendTxt html input box, append to the rxDataDiv in orange and call /send api (POST)
async function sendDataClick() {
    let s = autoUppercaseCB.checked ? serDataSendTxt.value.toUpperCase() : serDataSendTxt.value;
    rxDataDiv.innerHTML += '<span style="color:orange">' + escape(s) + "</span><br/>";

    const resp = await fetch("/send", {
        method: 'POST',
        headers: {
            'Content-Type': 'application/x-www-form-urlencoded'
        },
        body: encodeURIComponent('data') + '=' + encodeURIComponent(s)
    });
}

// allow to send from serDataSendTxt through enter key
async function serDataSendKeyDown(e) {
    if (e.keyCode === 13) {
        sendDataClick();
    }
}

// retrieve config at start through /config api (GET) json
fetch("/config").then(response => response.json()).then(data => {
    serSpeedTxt.value = data.speed;
});