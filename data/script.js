var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', onload);
function onload(event) {
    initWebSocket();
}

function getValues() {
    websocket.send("getValues");
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
    getValues();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

////////////////////// S

function updateOnOff(element) {
    if (document.getElementById("obj2").innerHTML == "ON") {
        document.getElementById("objValue2").innerHTML = "on";
        document.getElementById("obj2").innerHTML = "OFF";
        websocket.send("002s" + "on".toString());
    } else {
        document.getElementById("objValue2").innerHTML = "off";
        document.getElementById("obj2").innerHTML = "ON";
        websocket.send("002s" + "off".toString());
    }
}

function updateMaxTemperature(element) {
    var maxTempValue = document.getElementById('maxInput').value;
    websocket.send("003s" + maxTempValue.toString());
}

function addChartRow() {
    const div = document.createElement('div');

    div.className = 'row';

    div.innerHTML = `
      <label for="Temperatura">Temperatura:</label><br>
      <input class="temperaturaInput" type="number" name="Temperatura" value="0" />
      <label for="Velocidade">Velocidade:</label><br>
      <input class="velocidadeInput" type="number" name="Velocidade" value="0" />
      <br>
      <input type="button" value="-" onclick="removeChartRow(this)" />
    `;

    document.getElementById('chartParamBox').appendChild(div);
}

function removeChartRow(input) {
    document.getElementById('chartParamBox').removeChild(input.parentNode);
}

function okChartRow() {
    rowQtd = document.getElementsByClassName('row').length;
    let temperatura = document.getElementsByClassName('temperaturaInput');
    let velocidade = document.getElementsByClassName('velocidadeInput');

    let newDataList = [];
    for (let index = 0; index < rowQtd; index++) {
        if (temperatura[index].value != "")
            newDataList.push([temperatura[index].value, Number(velocidade[index].value)]);
    }

    websocket.send("006s" + newDataList.toString());
}

function cleanChartStatus() {
    document.getElementById('chartContainer').replaceChildren();
    document.getElementById('chartStatusBox').replaceChildren();
    websocket.send("006s");
}

function updateChartStatusBox() {
    if (controll006 != "") {
        document.getElementById('chartParamBox').replaceChildren();
        let arrayData = reArray(controll006);
        console.log(arrayData);
        arrayData.forEach(element => {
            document.getElementById("chartStatusBox").insertAdjacentHTML("beforeend", `
            <div class="statusRow">
                [${element[0]}, ${element[1]}]
                <br>
            </div>
            `);
        });
    }
};

var reArray = (stdList) => {
    let finalList = [];
    stdList = stdList.split(",");
    stdList.forEach((element, index, self) => {
        if (index % 2 === 0)
            finalList.push([self[index], Number(self[index + 1])]);
    });

    return finalList;
};

//anychart.onDocumentReady(() => loadChart());

const loadChart = (element) => {
    //re-array data
    let data = reArray(element);

    // create a data set
    var dataSet = anychart.data.set(data);

    // map the data for all series
    var firstSeriesData = dataSet.mapAs({ x: 0, value: 1 });

    // create a line chart
    var chart = anychart.line();

    // create the series and name them
    var firstSeries = chart.line(firstSeriesData);
    firstSeries.name("Temperatura");

    // add a legend
    chart.legend().enabled(true);

    // add a title
    chart.title("Gráfico de temperatura no tempo");

    // delete previous chart
    document.getElementById('chartContainer').replaceChildren();

    // specify where to display the chart
    chart.container("chartContainer");

    // draw the resulting chart
    chart.draw();
};


////////////////////// E

function updateObjPWM(element) {
    var objNumber = element.id.charAt(element.id.length - 1);
    var objValue = document.getElementById(element.id).value;
    document.getElementById("objValue" + objNumber).innerHTML = objValue;

    websocket.send(objNumber + "s" + objValue.toString());
}

let controll001;
let controll002;
let controll003;
let controll004;
let controll005;
let controll006 = "";

function onMessage(event) {
    var myObj = JSON.parse(event.data);
    var keys = Object.keys(myObj);

    for (var i = 0; i < keys.length; i++) { // auto atualização de objeto, tipo slider
        var key = keys[i];

        if (keys[i] == "objValue001") {
            // Slider
            document.getElementById(key).innerHTML = myObj[key];
            document.getElementById("obj001").value = myObj[key];
        }
        else if (keys[i] == "objValue002") {
            // ON/OFF
            document.getElementById(key).innerHTML = myObj[key];
            document.getElementById("obj001").value = myObj[key];
        }
        else if (keys[i] == "objValue003") {
            // Máxima
            if (myObj[key] != controll003) {
                document.getElementById(key).innerHTML = myObj[key];
                controll003 = myObj[key];
            }
        }
        else if (keys[i] == "objValue004") {
            // Atual
            if (myObj[key] != controll004) {
                document.getElementById(key).innerHTML = myObj[key];
                controll004 = myObj[key];
            }
        }
        else if (keys[i] == "objValue005") {
            // Rele Status
            if (myObj[key] != controll005) {
                document.getElementById(key).innerHTML = myObj[key];
                controll005 = myObj[key];
            }
        }
        else if (keys[i] == "objValue006") {
            // Chart
            let arrayString = myObj[key].toString();
            if (arrayString != controll006) {
                if (arrayString != "")
                    loadChart(arrayString);
                controll006 = arrayString;
                updateChartStatusBox();
            }
        }
        else {
            console.log("keys[i]: ");
            console.log(keys[i]);
            console.log("myObj[key]: ");
            console.log(myObj[key]);
        }
    }
}
