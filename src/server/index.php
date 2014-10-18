<?php
header('Content-Type: text/plain; charset=utf-8');

$payload = json_decode(file_get_contents('php://input'), true);
if(!$payload) {
	if (!isset($_GET['addr'])) throw_error("No address specified.");
	$address = $_GET['addr'];
} else {
	$address = $payload[1];
}

//// ImagenBTC ////

$total_url = "https://blockchain.info/q/addressbalance/" . $address;
$json_total = file_get_contents($total_url);
$total = json_decode($json_total) / 100000.0;

// HTTP response
echo json_encode(Array("1" => "$total mBTC"), JSON_UNESCAPED_UNICODE);
?>
