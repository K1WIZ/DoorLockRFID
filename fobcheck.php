/*
Dependencies: mosquitto-client, MySQL, PHP
Written By: John Rogers
License: GPL
Contact: john at wizworks dot net
*/

<?php

$search = $_POST['search'];
// DB Credentials
$servername = "127.0.0.1";
$username = "";
$password = "";
$dbname = "";

// MQTT Credentials
$pubHost = "";
$pubUser = "";
$pubPass = "";
$topic = "domoticz/in";

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);

// Check connection
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

$q1    = "SELECT active FROM tags WHERE tag = '$search' LIMIT 1;";
$result = mysqli_query($conn, $q1);

if (!$result) {
    echo "DB Error, could not query the database\n";
    echo 'MySQL Error: ' . mysqli_error();
    exit;
}

while ($row = mysqli_fetch_assoc($result)) {
    foreach ($row as $field)
    if ( $field == '1' ) {
    echo shell_exec(`mosquitto_pub -h $pubHost -u $pubUser -P $pubPass -t $topic -m "{\"command\":\"switchlight\",\"idx\":123,\"switchcmd\":\"Off\"}"`);
    echo "SPANK";
    } elseif ( $field == '0' ) { 
    echo "INVALID";
    } elseif ( $field == ' ' ) {
    echo "INVALID";
    }
}

mysqli_free_result($result);

?>
