<?php
$DEBUG=$_GET["DEBUG"];

if ($DEBUG) {
	echo "<!DOCTYPE html><html><meta charset='utf-8'><link rel='stylesheet' type='text/css' href='weather-icons/css/weather-icons.min.css'></head><body class='wi'>";
	error_reporting(-1);
	ini_set('display_errors', 'On');
	// print_r($_GET);
} else {
	header('Content-Type: text/plain; charset=utf-8');
}

function throw_error($msg) {
	die('{5:"' . $msg . '"}');
}

$icons = Array(
	"01d" => 0xf00d, // "sky is clear",
	"02d" => 0xf00c, // "few clouds",
	"03d" => 0xf002, // "scattered clouds",
	"04d" => 0xf013, // "broken clouds",
	"09d" => 0xf018, // "shower rain",
	"10d" => 0xf019, // "rain",
	"11d" => 0xf01d, // "thunderstorm",
	"13d" => 0xf01b, // "snow",
	"50d" => 0xf014, // "mist",
	
	"01n" => 0xf02e, // "sky is clear - night",
	"02n" => 0xf031, // "few clouds - night",
	"03n" => 0xf031, // "scattered clouds - night",
	"04n" => 0xf013, // "broken clouds - night",
	"09n" => 0xf029, // "shower rain - night",
	"10n" => 0xf028, // "rain - night",
	"11n" => 0xf02c, // "thunderstorm - night",
	"13n" => 0xf01b, // "snow - night",
	"50n" => 0xf04a, // "mist - night",
	
	"" => ""
);

$payload = json_decode(file_get_contents('php://input'), true);
if(!$payload) {
	if (!isset($_GET['lat'])) throw_error("No lattitude ('lat') specified.");
	if (!isset($_GET['lon'])) throw_error("No longitude ('lon') specified.");
	if (!isset($_GET['unt'])) throw_error("No units ('unt' = 'metric') specified.");
	$lat = $_GET['lat'] / 10000;
	$lon = $_GET['lon'] / 10000;
	$unt = $_GET['unt'];
} else {
	$lat = $payload[1] / 10000;
	$lon = $payload[2] / 10000;
	$unt = $payload[3];
}

//// Torino ////
// $lat = "45.075945";
// $lon = "7.686256";
// $unt = "metric";

// error_log($lat . " " . $lon . " " . $unt);

$current_url = 'http://api.openweathermap.org/data/2.5/weather?lat='.$lat.'&lon='.$lon.'&units='.$unt;
$json_curr = curl_get($current_url);
// if (!$json_curr) throw_error("Bad openweathermap result.");
$weather = process_weather($json_curr, $icons, $DEBUG);

$loc_url = "http://open.mapquestapi.com/nominatim/v1/search?q=$lat,$lon&format=json&addressdetails=1";
$loc_raw = curl_get($loc_url);
$json_loc = json_decode($loc_raw);
// print_r($json_loc);
// if (!$json_loc) throw_error("Bad mapquest address.");

/** Uncomment to get approx 3 hours ahead forecast **/
// $json_forc = json_decode(curl_get('http://api.openweathermap.org/data/2.5/forecast?cnt=3&mode=json&lat='.$lat.'&lon='.$lon.'&units='.$unt));
//$weather[1] = html_entity_decode($icons[$json_forc->list[2]->weather[0]->icon], 0, 'UTF-8');
//$weather[2] = array('I', round($json_forc->list[2]->main->temp, 0));

// debug - print out each time segment forecast (3 hourly)
/*foreach ($json_forc->list as $time_segment) {
	print_r($time_segment);
	print "<br/>";
}*/

// add current locale and custom server message
foreach (Array("city", "village", "county", "state", "country") as $address_key) {
	if (array_key_exists($address_key, $json_loc[0]->address)) {
		$weather[3] = $json_loc[0]->address->{$address_key};
		break;
	}
}
$weather[4] = file_get_contents("message.txt");
if ($weather[4] === false) {
	$weather[4] = "";
}

// HTTP response
print json_encode($weather, JSON_UNESCAPED_UNICODE);

// make a record of the last weather value requested
file_put_contents("weather-icon.txt", sprintf("%x\n", $weather[1][1]));

function curl_get($url){
	if (!function_exists('curl_init')){
		die('Sorry cURL is not installed!');
	}
	$ch = curl_init();
	curl_setopt($ch, CURLOPT_URL, $url);
	curl_setopt($ch, CURLOPT_REFERER, $url);
	curl_setopt($ch, CURLOPT_USERAGENT, "Mozilla/1.0");
	curl_setopt($ch, CURLOPT_HEADER, 0);
	curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
	curl_setopt($ch, CURLOPT_TIMEOUT, 10);
	$output = curl_exec($ch);
	curl_close($ch);
	return $output;
}

function process_weather($json_curr, $icons, $DEBUG) {
	$json_output = json_decode($json_curr);
	if (!$json_output) {
		$weather = "";
		$temp = -127;
		$icon = "";
	} else {
		$weather = $json_output->weather;
		$temp = $json_output->main->temp;
		$icon = $weather[0]->icon;
	}
	
	// print $json_curr;

	//$json_output = json_decode(utf8_decode($json_forc));
	//$list		= $json_output->list;
	//$day		 = $list[0]->temp;
	//$temp_min	= $day->min;
	//$temp_max	= $day->max;

	// log_error("Weather Icon:" . $icon);
	if ($DEBUG) {
		echo $icon . "<br/>";
		echo $icons[$icon] . "<br/>";
	}
	
	$result	= array();
	$result[1] = array("I", $icons[$icon]); // html_entity_decode("&#x" . $icons[$icon] . ";", 0, 'UTF-8');
	$result[2] = array('b', round($temp, 0));
	// $result[6] = $icons[$icon];
	// $result[3] = array('I', round($temp_min, 0));
	// $result[4] = array('I', round($temp_max, 0));
	return $result;
}

if ($DEBUG) {
	echo "</body></html>";
}
?>
