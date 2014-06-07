<?
$rates = json_decode(file_get_contents("https://bitpay.com/api/rates?_=" . rand()));
echo sprintf("%.1f", $rates[0]->{"rate"}); // . "(" . $rates[0]->{"code"} . ")";
?>
