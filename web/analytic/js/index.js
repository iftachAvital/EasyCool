var local_username;
var local_password;

var loader_div = document.getElementById('loader');
var signin_div = document.getElementById('signin-div');

var signin_email = document.getElementById('signin-email');
var signin_password = document.getElementById('signin-password');
var signin_button = document.getElementById('signin-button');
var signin_alert_box = document.getElementById('signin-alert-box');
var signin_alert_box_text = document.getElementById('signin-alert-box-text');

var main_div = document.getElementById('w');
var app_loader = document.getElementById('app-loader');
var app_div = document.getElementById('app-div');

var toolbar_title = document.getElementById('toolbar-title');

var graph_canvas = document.getElementById('graph-chart');
var graph_device_select = document.getElementById('graph-device-select');

var connectivity_problem_medal = document.getElementById('connectivity-problem-medal');

var users_data;

var showLoader = function() {
	loader_div.style.display = 'inline';
	main_div.className = 'hidden';
	signin_div.style.display = 'none';
};

var showAppLoader = function() {
	signin_div.style.display = 'none';
	main_div.className = '';
	app_div.className = 'hidden';
	loader_div.style.display = 'none';
	app_loader.style.display = 'inline';
};

var showSignin = function() {
	signin_button.disabled = false;
	main_div.className = 'hidden';
	loader_div.style.display = 'none';
	signin_div.style.display = 'inline';
	signin_alert_box.style.display = 'none';
	signin_email.value = "";
	signin_password.value = "";
};

var showApp = function() {
	signin_div.style.display = 'none';
	main_div.className = '';
	app_div.className = '';
	loader_div.style.display = 'none';
	app_loader.style.display = 'none';
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

var handleApiError = function(result, callback) {
	console.log(result);

	if (result.status == 401) {
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

signin_button.onclick = function() {
	local_username = signin_email.value;
	local_password = signin_password.value;

	if (local_password && local_username) {
		signin_button.disabled = true;
		showLoader();
		loadUsers(local_username, local_password);
	}
	else {
		signin_alert_box_text.innerHTML = 'Missing username or password';
		signin_alert_box.style.display = 'inline';
	}
};

var loadUsers = function(username, password) {
	showAppLoader();

	var apigClient = apigClientFactory.newClient();

	var body = {
		username: username,
		password: password,
		command: 'get_users'
	};

	apigClient.analyticPost({}, JSON.stringify(body), {})
		.then(function(result){
			console.log(result.data);
			users_data = result.data;

			localStorage.setItem("username", username);
			localStorage.setItem("password", password);

			handleUsers(result.data);
		}).catch( function(result){
			handleApiError(result, appStartUp);
		});
};

var handleUsers = function(data) {
	for(var i = 0; i < data.Count; i++) {
		var new_option = document.createElement('option');
		new_option.text = data.Items[i].UserName;
		graph_device_select.appendChild(new_option);
	}

	handleOnSelectUser();
	showApp();
};

var logout = function() {
	localStorage.clear();
	local_password = null;
	local_username = null;
	showSignin();
};

var appStartUp = function() {
	local_username = localStorage.getItem("username");
	local_password = localStorage.getItem("password");

	if (local_password === null || local_username === null) {
		showSignin();
	}
	else {
		loadUsers(local_username, local_password);
	}
};

var options = {
	scaleShowVerticalLines: false,
	scales: {
		yAxes: [{
			type: 'linear',
			ticks: {
                beginAtZero:true
            }
		}],
		xAxes: [{
			type: 'time',
			time: {
				unit: 'day'
			},
			display: true,
			gridLines : {
                display : false
            }
		}]
	}
};

var graph_chart = Chart.Line(graph_canvas, {
	data: {
		datasets: [],
	},
	options: options
});

var addNewData = function(data, name, color) {
	graph_chart.data.datasets.push({
		label: name,
		fill: false,
		lineTension: 0.05,
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
};

var handleQueryResult = function(data, from, to) {
	var app_data_set = [{x: moment.unix(from).second(0).minute(0).hour(0)}, {x: moment.unix(to).second(0).minute(0).hour(0)}];
	var sms_data_set = [{x: moment.unix(from).second(0).minute(0).hour(0)}, {x: moment.unix(to).second(0).minute(0).hour(0)}];

	var days = Math.ceil(to - from) / (60*60*24);
	var j = 0;
	var k = 0;

	for (var i = 0; i <= days; i++) {
		var app_data_point = {x: moment.unix(from + (60*60*24*i)).second(0).minute(0).hour(0), y:0};
		var sms_data_point = {x: moment.unix(from + (60*60*24*i)).second(0).minute(0).hour(0), y:0};

		while (j < data.app.Count && app_data_point.x.isSame(moment.unix(data.app.Items[j].LogTime), 'day')) {
			app_data_point.y++;
			j++;
		}

		while (k < data.sms.Count && sms_data_point.x.isSame(moment(data.sms.Items[k].ChangedAt), 'day')) {
			if (data.sms.Items[k].SMSSent == 0) {
				sms_data_point.y++;
			}
			else {
				sms_data_point.y += data.sms.Items[k].SMSSent;
			}
			k++;
		}

		app_data_set.push(app_data_point);
		sms_data_set.push(sms_data_point);
	}

	addNewData(app_data_set, 'App use', '#377eb8');
	addNewData(sms_data_set, 'SMS use', '#e41a1c');

	showApp();
	graph_chart.update();
};

var getUserLogQuery = function(user, from, to) {
	showAppLoader();

	var apigClient = apigClientFactory.newClient();

	var body = {
		username: local_username,
		password: local_password,
		command: 'get_use',
		user: user,
		from: from,
		to: to
	};

	apigClient.analyticPost({}, JSON.stringify(body), {})
		.then(function(result){
			console.log(result.data);
			handleQueryResult(result.data, from, to);

		}).catch( function(result){
			handleApiError(result, handleOnSelectUser);
		});
};

var handleOnSelectUser = function() {
	var i = graph_device_select.selectedIndex;
	var time_in_sec = Math.round(Date.now() / 1000);
	console.log(users_data['Items'][i].UserName);

	graph_chart.data.datasets = [];
	getUserLogQuery(users_data['Items'][i].UserName, (time_in_sec - (60*60*24*7)), time_in_sec);
}
