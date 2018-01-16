var DEVICE_TYPE_REFRIGERATOR = 0;
var DEVICE_TYPE_FREEZER = 1;
var DEVICE_TYPE_ICE_CREAM = 2;

var page_timer;
var local_username;
var local_password;

var loader_div = document.getElementById('loader');
var signin_div = document.getElementById('signin-div');

var signin_email = document.getElementById('signin-email');
var signin_password = document.getElementById('signin-password');
var signin_button = document.getElementById('signin-button');
var signin_alert_box = document.getElementById('signin-alert-box');
var signin_alert_box_text = document.getElementById('signin-alert-box-text');

var app_div = document.getElementById('w');
var status_div = document.getElementById('status-div');
var status_canvas = document.getElementById('status-chart');
var devices_div = document.getElementById('devices-div');
var graph_div = document.getElementById('graph-div');
var graph_canvas = document.getElementById('graph-chart');
var device_settings_div = document.getElementById('device-settings-div');
var user_settings_div = document.getElementById('user-settings-div');
var empty_div = document.getElementById('empty-div');
var connectivity_problem_medal = document.getElementById('connectivity-problem-medal');

var toolbar_title = document.getElementById('toolbar-title');

var devices_list;

var new_user_flag = false;
// display functions

var showLoader = function() {
	loader_div.style.display = 'inline';
	app_div.className = 'hidden';
	signin_div.style.display = 'none';
};

var showSignin = function() {
	signin_button.disabled = false;
	app_div.className = 'hidden';
	loader_div.style.display = 'none';
	signin_div.style.display = 'inline';
	signin_alert_box.style.display = 'none';
	signin_email.value = "";
	signin_password.value = "";
};

var showStatus = function() {
	signin_div.style.display = 'none';
	app_div.className = '';
	loader_div.style.display = 'none';
	status_div.className = '';
	devices_div.style.display = 'none';
	graph_div.className = 'hidden';
	device_settings_div.style.display = 'none';
	user_settings_div.style.display = 'none';
	toolbar_title.innerHTML = 'Status';
	empty_div.style.display = 'none';
};

var showDevices = function() {
	app_div.className = '';
	loader_div.style.display = 'none';
	status_div.className = 'hidden';
	devices_div.style.display = 'inline';
	graph_div.className = 'hidden';
	device_settings_div.style.display = 'none';
	user_settings_div.style.display = 'none';
	toolbar_title.innerHTML = 'Devices';
	empty_div.style.display = 'none';
};

var showGraph = function () {
	app_div.className = '';
	loader_div.style.display = 'none';
	status_div.className = 'hidden';
	devices_div.style.display = 'none';
	graph_div.className = '';
	device_settings_div.style.display = 'none';
	user_settings_div.style.display = 'none';
	toolbar_title.innerHTML = 'Graph';
	empty_div.style.display = 'none';
};

var showDeviceSettings = function() {
	app_div.className = '';
	loader_div.style.display = 'none';
	status_div.className = 'hidden';
	devices_div.style.display = 'none';
	graph_div.className = 'hidden';
	device_settings_div.style.display = 'inline';
	user_settings_div.style.display = 'none';
	toolbar_title.innerHTML = 'Device Settings';
	empty_div.style.display = 'none';
};

var showUserSettings = function() {
	app_div.className = '';
	loader_div.style.display = 'none';
	status_div.className = 'hidden';
	devices_div.style.display = 'none';
	graph_div.className = 'hidden';
	device_settings_div.style.display = 'none';
	user_settings_div.style.display = 'inline';
	toolbar_title.innerHTML = 'User Settings';
	empty_div.style.display = 'none';
};

var showEmpty = function() {
	app_div.className = '';
	loader_div.style.display = 'none';
	status_div.className = 'hidden';
	devices_div.style.display = 'none';
	graph_div.className = 'hidden';
	device_settings_div.style.display = 'none';
	user_settings_div.style.display = 'none';
	empty_div.style.display = 'inline';
};

var callErrorMedal = function(callback, title, message) {
	document.getElementById("error-medal-title").innerHTML = title;
	document.getElementById("error-medal-message").innerHTML = message;
	connectivity_problem_medal.style.display = "inline";
	document.getElementById('connectivity-problem-medal-button').onclick = function() {
		connectivity_problem_medal.style.display = "none";
		callback();
	}
};

// user authentication functions

var handleApiError = function(result, callback) {
	console.log(result);
		
	if (result.status == 400) {
		handleUnauthorized();
	}
	else {
		callErrorMedal(callback, "Sorry!", "We couldn't connect to the server.");
	}
};

var handleUnauthorized = function() {
	console.log("unauthorized");
	localStorage.clear();

	showSignin();
	signin_alert_box_text.innerHTML = 'Wrong username or password';
	signin_alert_box.style.display = 'inline';
};

var callDevices = function(username, password) {
	var apigClient = apigClientFactory.newClient();
	
	var body = {
		username: username,
		password: password,
		command: 'get_devices'
	};

	apigClient.appPost({}, JSON.stringify(body), {})
		.then(function(result){
			console.log(result.data);
			localStorage.setItem("username", username);
			localStorage.setItem("password", password);
			devices_list = result.data;
			getDevices();
		}).catch( function(result){
			handleApiError(result, getStatus);
		});	
};

signin_button.onclick = function() {
	local_username = signin_email.value;
	local_password = signin_password.value;
	
	if (local_password && local_username) {
		signin_button.disabled = true;
		showLoader();
		callDevices(local_username, local_password);
	}
	else {
		signin_alert_box_text.innerHTML = 'Missing username or password';
		signin_alert_box.style.display = 'inline';
	}
};

// status and devices list functions and vars

var disconnected_devices_div = document.getElementById('disconnected-devices-div');
var alert_devices_div = document.getElementById('alert-devices-div');
var connected_devices_div = document.getElementById('connected-device-div');
var sensor_fault_devices_div = document.getElementById('sensor-fault-devices-div');

var connectionChart  = new Chart(status_canvas, {
	type: 'doughnut',
	data: {
		labels: [
			"connected",
			"temperature alert",
			"disconnected"
		],
		datasets: [{
			data: [0,0,0],
			backgroundColor: [
				"#36A2EB",
				"#FF6384",
				"#FFCE56"
			],
			hoverBackgroundColor: [
				"#36A2EB",
				"#FF6384",
				"#FFCE56"
			]
		}]
	},
	options: {
		title: {
			display: true,
			text: "EasyCool Status"
		}
	}
});

function clearDevicesData() {
	connected_devices_div.innerHTML = '';
	disconnected_devices_div.innerHTML = '';
	alert_devices_div.innerHTML = '';
	sensor_fault_devices_div.innerHTML = '';
	connectionChart.data.datasets[0].data = [0,0,0];
	connectionChart.update();
}

function handleDeviceInfo(item) {
	if (item.DeviceStatus == 3) {
		connectionChart.data.datasets[0].data[2]++;
	}
	else {
		connectionChart.data.datasets[0].data[item.DeviceStatus]++;
	}
	
	connectionChart.update();
	
	var parent;
	var m_blc = document.createElement("div");
	m_blc.className = "m-blc";
	var row = document.createElement("div");
	row.className = "row";
	var xs_8 = document.createElement("div");
	xs_8.className = "col-xs-7 ydevics";
	var a = document.createElement("a");
	a.innerHTML = item.DeviceName;
	xs_8.appendChild(a);
	row.appendChild(xs_8);
	var xs_2 = document.createElement("div");
	xs_2.className = "col-xs-3";
	var po = document.createElement("div");
	po.className = "po-relative";
	if(item.LastTemp != null) {
		po.innerHTML = item.LastTemp.toString() + '°';
	}
	xs_2.appendChild(po);
	row.appendChild(xs_2);
	var xs_2_2 = document.createElement("div");
	xs_2_2.className = "col-xs-2";
	var ic = document.createElement("div");
	ic.className = "ic-notconnect";
	var img = document.createElement("img");
	
	switch(item.DeviceStatus) {
		case 0:
			parent = connected_devices_div;
			img.src = "images/done.png";
			break;
		case 1:
			parent = alert_devices_div;
			img.src = "images/warning.png";
			break;
		case 2:
			parent = disconnected_devices_div;
			img.src = "images/notconnect.png";
			po.innerHTML = ""; //TODO: decide if to show the last temperature also if the device if define disconncted
			break;
		case 3:
			parent = sensor_fault_devices_div;
			img.src = "images/sensor_fault.jpeg";
			po.innerHTML = ""; //TODO: decide if to show the last temperature also if the device if define disconncted
			break;
	}

	ic.appendChild(img);
	xs_2_2.appendChild(ic);
	row.appendChild(xs_2_2);
	m_blc.appendChild(row);
	m_blc.onclick = function() {
		getDeviceSettings(item);
	}
	parent.appendChild(m_blc);
}

function getDevices() {
	showLoader();
	clearDevicesData();

	for(var i = 0; i < devices_list.Count; i++) {
		handleDeviceInfo(devices_list.Items[i]);
	}

	showStatus();
}

// device settings functions and vars

var device_settings_name_input = document.getElementById('device-settings-name-input');
var device_settings_current_temp = document.getElementById('device-settings-currrent-temp');
var device_settings_type_div = document.getElementById('device-settings-type-div');
var device_settings_idview = document.getElementById('device-id-view');
var device_settings_save_button = document.getElementById('device-settings-save-button');
var device_settings_delete_button = document.getElementById('device-settings-delete-button');
var device_settings_graph_image = document.getElementById('device-settings-graph-image');
var device_settings_network_name = document.getElementById('device-settings-network-name');
var device_settings_network_signal = document.getElementById('device-settings-network-signal');
var device_settings_network_div = document.getElementById('device-settings-network-div');
var device_Settings_wifi_img = document.getElementById('device-settings-wifi-image');

function getDeviceSettings(device) {
	device_settings_graph_image.onclick = function() {
		getGraph(device.DeviceId);
	}

	device_settings_idview.innerHTML = device.DeviceId;
	getDeviceInfo(device);
}

function getRSSIasQuality(RSSI) {
    var quality = 0;

    if (RSSI <= -100) {
        quality = 0;
    } else if (RSSI >= -50) {
        quality = 100;
    } else {
        quality = 2 * (RSSI + 100);
    }
    return quality;
}

function getDeviceInfo(device) {
	device_settings_name_input.innerHTML = device.DeviceName;
	old_device_name = device.DeviceName;
	
	if(device.DeviceStatus != 2 && device.DeviceStatus != 3 && device.LastTemp != null) {
		device_settings_current_temp.innerHTML = device.LastTemp.toString() + '°';
		if (device.DeviceRSSI != null) {
			device_settings_network_name.innerHTML = device.DeviceSSID;

			var qual = getRSSIasQuality(device.DeviceRSSI);
			console.log('qual', qual);

			if (qual >= 75) {
				device_Settings_wifi_img.src = 'images/wifi1.jpeg';
			}
			else if (qual < 75 && qual >= 50) {
				device_Settings_wifi_img.src = 'images/wifi2.jpeg';
			}
			else if (qual < 50 && qual >= 25) {
				device_Settings_wifi_img.src = 'images/wifi3.jpeg';
			}
			else if (qual < 25) {
				device_Settings_wifi_img.src = 'images/wifi4.jpeg';
			}
		}
		else {
			device_settings_network_name.innerHTML = '';
			device_Settings_wifi_img.src = '';
		}
	}
	else {
		device_settings_network_name.innerHTML = '';
		device_Settings_wifi_img.src = '';
		device_settings_current_temp.innerHTML = '';
	}

	switch(device.DeviceType) 
	{
		case 0:
			device_settings_type_div.innerHTML = 'Refrigerator';
			break;
		case 1:
			device_settings_type_div.innerHTML = 'Freezer';
			break;
		case 2:
			device_settings_type_div.innerHTML = 'Ice Cream Freezer';
			break;

	}

	showDeviceSettings();
}
	
// user settings functions and vars

var user_settings_phones_div = document.getElementById('user-settings-phones-div');

function logout() {
	localStorage.clear();
	local_password = null;
	local_username = null;
	showSignin();
}

var getUserSettings = function() {
	showEmpty();
	getPhoneNumbers();	
};

function getPhoneNumbers() {	
	var apigClient = apigClientFactory.newClient();
	
	var body = {
		username: local_username,
		password: local_password,
		command: 'get_phones'
	};

	apigClient.appPost({}, JSON.stringify(body), {})
		.then(function(result){
			console.log(result.data);
			handleGetPhones(result.data.phone_numbers);
		}).catch( function(result){
			handleApiError(result, getUserSettings);
		});	
}

function createPhoneNumberItem(parent, phone_num, delete_button) {
	var main_div = document.createElement('div');
	main_div.className = 'form-group';
	var row_div = document.createElement('div');
	row_div.className = 'row';
	var xs_8_div = document.createElement('div');
	xs_8_div.className = 'col-xs-8';
	var input = document.createElement('input');
	input.type = 'tel';
	input.className = 'form-control';
	input.name = 'phone_number[]';
	input.value = phone_num;
	xs_8_div.appendChild(input);
	row_div.appendChild(xs_8_div);
	if(delete_button) {
		var xs_4_div = document.createElement('div');
		xs_4_div.className = 'col-xs-4';
		var a = document.createElement('a');
		a.className = 'ic-remove';
		a.onclick = function() {
			parent.removeChild(main_div);
		}
		var i = document.createElement('i');
		i.className = 'fa fa-close';
		a.appendChild(i);
		xs_4_div.appendChild(a);
		row_div.appendChild(xs_4_div);
	}
	main_div.appendChild(row_div);
	parent.appendChild(main_div);	
}

function addNewPhoneNumber() {
	var phone_numbers = document.getElementsByName('phone_number[]');
	if(phone_numbers.length < 3) {
		createPhoneNumberItem(user_settings_phones_div, '', true);
	}
	else {
		alert('maximun 3 numbers');
	}
}

function handleGetPhones(phone_numbers) {
	user_settings_phones_div.innerHTML = '';
	switch(phone_numbers.length) {
		case 0:
			createPhoneNumberItem(user_settings_phones_div, '', false);
			break;
		case 1:
			createPhoneNumberItem(user_settings_phones_div, phone_numbers[0], false);
			break;
		case 2:
			createPhoneNumberItem(user_settings_phones_div, phone_numbers[0], true);
			createPhoneNumberItem(user_settings_phones_div, phone_numbers[1], true);
			break;
		case 3:
			createPhoneNumberItem(user_settings_phones_div, phone_numbers[0], true);
			createPhoneNumberItem(user_settings_phones_div, phone_numbers[1], true);
			createPhoneNumberItem(user_settings_phones_div, phone_numbers[2], true);
			break;
		default:
			createPhoneNumberItem(user_settings_phones_div, '', false);
			break;
	}
	showUserSettings();
}

function userSettingsSavePhones() {
	showEmpty();
	
	var phone_numbers = document.getElementsByName('phone_number[]');
	var array_to_send = [];
	
	for(var i=0; i < phone_numbers.length; i++) {
		if(phone_numbers[i].value.length == 10 && phone_numbers[i].value.indexOf('05') == 0) {
			array_to_send.push({'number': phone_numbers[i].value});
		}
	}
	
	if (array_to_send.length < 1) {
		callErrorMedal(getUserSettings, "Error", "Invalid phone number");
		return;
	}
		
	var apigClient = apigClientFactory.newClient();
	
	var body = {
		username: local_username,
		password: local_password,
		command: 'save_phones',
		phone_numbers: array_to_send
	};
	
	console.log(JSON.stringify(body));

	apigClient.appPost({}, JSON.stringify(body), {})
		.then(function(result){
			console.log(result.data);
			getUserSettings();
		}).catch( function(result){
			handleApiError(result, getUserSettings);
		});	
}

// graph functions and vars

var graph_device_select = document.getElementById('graph-device-select');
var graph_button_next = document.getElementById('graph-button-next');
var graph_button_back = document.getElementById('graph-button-back');
var graph_date_text = document.getElementById('graph-date-p');

var graph_time;
var graph_time_jumps = (8*60*60*1000);

var options = {
	scaleShowVerticalLines: false,
	scales: {
		yAxes: [{
			type: 'linear',
			ticks: {
				stepSize: 2
			}
		}],
		xAxes: [{
			type: 'time',
			time: {
				unit: 'hour',
				displayFormats: {
					hour: 'HH:MM'
				}
			},
			display: true,
			gridLines : {
                display : false
            }
		}]
	},
	legend: {
		display: false
	}
};

var graph_chart = Chart.Line(graph_canvas, {
	data: {
		datasets: [],
	},
	options: options
});

function addNewData(data, name, min, max, color) {
	graph_chart.data.datasets.push({
		label: name,
		fill: false,
		lineTension: 0.1,
		backgroundColor: color,
		borderColor: color,
		borderCapStyle: 'butt',
		borderDash: [],
		borderDashOffset: 0.0,
		borderJoinStyle: 'miter',
		pointBorderColor: color,
		pointBackgroundColor: "#fff",
		pointBorderWidth: 1,
		pointHoverRadius: 5,
		pointHoverBackgroundColor: color,
		pointHoverBorderColor: "rgba(220,220,220,1)",
		pointHoverBorderWidth: 2,
		pointRadius: 1,
		pointHitRadius: 10,
		data: data
	});
	
	graph_chart.options.scales.yAxes[0].ticks.max = max;
	graph_chart.options.scales.yAxes[0].ticks.min = min;
}

function handleQueryResult(data, name, from, to, type) {
	console.log(data);
	var min;
	var max;
	
	switch (type)
	{
		case DEVICE_TYPE_REFRIGERATOR:
			min = 0;
			max = 8;
			break;
		case DEVICE_TYPE_FREEZER:
			min = -18;
			max = -4;
			break;
		case DEVICE_TYPE_ICE_CREAM:
			min = -22;
			max = -14;
			break;
	}

	if (data.Count > 0) {
		var lastStatus = data.Items[0].DeviceStatus;
		var data_set = [{x: from}, {x: to}];

		for(var i = 0; i < data.Count; i++) {

			if (lastStatus != data.Items[i].DeviceStatus) {
				if (lastStatus == 0) {
					addNewData(data_set, name, min, max, '#404040');
				}
				else {
					addNewData(data_set, name, min, max, '#FF0000');
				}
				lastStatus = data.Items[i].DeviceStatus;
				data_set = [];
			}
			else {
				data_set.push({x: moment(data.Items[i].PubishAt), y: data.Items[i].Temperature});
			}

			if (data.Items[i].Temperature > max) {
				max = 2 * Math.ceil(data.Items[i].Temperature / 2);
			}
			if (data.Items[i].Temperature < min) {
				min = 2 * Math.floor(data.Items[i].Temperature / 2);
			}
		}

		if (lastStatus == 0) {
			addNewData(data_set, name, min, max, '#404040');
		}
		else {
			addNewData(data_set, name, min, max, '#FF0000');
		}	
	}
	else {
		var data_set = [{x: from}, {x: to}];
		addNewData(data_set, name, min, max, '#404040');
	}
	
	graph_date_text.innerHTML = moment(from).format(' D MMM YYYY');
	graph_chart.update();
}

function getDeviceTempQuery(device_id, from, to, name, type) {	
	var apigClient = apigClientFactory.newClient();
	
	var body = {
		username: local_username,
		password: local_password,
		command: 'temperature',
		device_id: device_id,
		from: from,
		to: to
	};

	apigClient.appPost({}, JSON.stringify(body), {})
		.then(function(result){
			console.log(result.data);
			
			showGraph();
			handleQueryResult(result.data, name, from, to, type);
			
		}).catch( function(result){
			handleApiError(result, getGraph);
		});	
}

function getGraph(device_id) {
	showEmpty();
	graph_device_select.innerHTML = '';
	
	if(devices_list != null) {
		for(var i = 0; i < devices_list.Count; i++) {
			var new_option = document.createElement('option');
			new_option.text = devices_list.Items[i].DeviceName;
			graph_device_select.appendChild(new_option);
		}
	}
	else {
		getDevices();
	}
	
	graph_time = Date.now();
	graph_button_next.disabled = true;
	
	graph_button_back.onclick = function() {
		graph_time -= graph_time_jumps;
		graph_button_next.disabled = false;
		getGraphByTime();
	}
	
	graph_button_next.onclick = function() {
		if(graph_time + graph_time_jumps > Date.now()) {
			graph_time = Date.now();
			getGraphByTime();
			graph_button_next.disabled = true;
		}
		else {
			graph_time += graph_time_jumps;
			getGraphByTime();
		}
	}
	
	if(device_id != null) {
		for(var i = 0; i < devices_list.Count; i++) {
			if(devices_list.Items[i].DeviceId.localeCompare(device_id) == 0) {
				graph_device_select.selectedIndex = i;
				break;
			}
		}
	}
	
	getGraphByTime();
}

function getGraphByTime() {
	showEmpty();
	graph_chart.data.datasets = [];
	var i = graph_device_select.selectedIndex;
	
	if(devices_list != null) {
		getDeviceTempQuery(devices_list.Items[i].DeviceId, graph_time - graph_time_jumps, graph_time, devices_list.Items[i].DeviceName, devices_list.Items[i].DeviceType);
	}
	else {
		getDevicesInfo();
	}
}

// app startup flow control

var getStatus = function() {
	showEmpty();
	appStartUp();
};

function appStartUp() {	
	local_username = localStorage.getItem("username");
	local_password = localStorage.getItem("password");
	
	if (local_password === null || local_username === null) {
		showSignin();
	}
	else {
		callDevices(local_username, local_password);
	}
}
