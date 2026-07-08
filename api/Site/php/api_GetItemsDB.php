<?php
	$serverkey = $_POST['serverkey'];
	$ServerMode = 0;
	if ($serverkey == "8B1E58D9-1D8A-4942-A2AB-B6809F0A4CDF")
	{
		$ServerMode = 1;
	}
	if ($serverkey == "9F179EB9-C74E-4933-85B5-EB135E16F5EF")
	{
		$ServerMode = 2;
	}
	if ($ServerMode == 0)
	{
		die('oops');
	}

	header("Content-type: text/xml");

	$itemsDbNew = __DIR__ . '/../../../bin/Data/Weapons/itemsDB_new.xml';
	if (!is_file($itemsDbNew))
	{
		http_response_code(500);
		die('itemsDB_new.xml is missing');
	}

	readfile($itemsDbNew);
	exit();
?>
