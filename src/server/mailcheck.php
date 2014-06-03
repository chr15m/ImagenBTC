<?
function get_mailbox_counts() {
	$boxes = parse_ini_file("mailboxes.conf", true);
	$counts = Array();

	foreach ($boxes as $name => $config) {
		$connect_string = "{" . $config{"server"} . ":" . $config{"port"} . "/imap/novalidate-cert" . ($config{"ssl"} ? "/ssl" : "") . "}INBOX";
		array_push($counts, mail_count($connect_string, $config{"username"}, $config{"password"}));
	}
	return join($counts, " ");
}

/* Helper functions. */

function mail_count($host, $username, $password) {
	$mbox = imap_open($host, $username, $password);
	if ($mbox) {
		$result = imap_search($mbox, 'UNSEEN');
		if ($result) {
			return count($result);
		} else {
			return 0;
		}
	}
}

echo get_mailbox_counts();
?>
