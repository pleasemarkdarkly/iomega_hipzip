<?php
// The section below is only loaded at the very beginning of any page.
// If this page has already been included, skip to below...

// $type, if defined, should be defined before calling this page.
// Likewise, the page should have a $pagetitle declared.

if (empty($isLoaded))
{
//phpinfo();
// necessary for IIS version:
if (empty($type)) {$type = "";}
if (empty($meta)) {$meta = "";}
if (empty($subpage)) {$subpage = "";}

	$href=strstr($PHP_SELF,"doxy/")? "../": "";
	include($href."scripts.php");

	if (GetFrameSet($PHP_SELF)=="d" && $type == "doxy") //we're in the Doxygen directory
		{
		$href="../";
		}

	echo <<<EOQ
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">

<html>
<head>
	<title>$pagetitle</title>

<SCRIPT LANGUAGE="JavaScript" SRC="
EOQ;
		echo($href . "io.js\"></SCRIPT>
<link rel=\"STYLESHEET\" type=\"text/css\" href=\"" . $href . "iObjectsStyle.css\">");

		if (!empty($style))
		{ echo ("<link rel=\"STYLESHEET\" type=\"text/css\" href=\"$style\">");
		}
		
		echo("$meta
</head>

<body lang=EN-US link=blue vlink=purple>
");

//End of section used on all pages.


	switch ($type)
	{
	case "":  //if undefined
	case "normal": //explicitly declared (should not be used for pages that may be in
	case "doxy":			// a summary!
		$currentHeader = getNav($PHP_SELF, $nextPage, $previousPage);	
		navBox($PHP_SELF,$nextPage,$previousPage);

		if ($type!="doxy")
		{
			print head(substr($currentHeader[0],0,strpos($currentHeader[0],".")),$currentHeader[1],$currentHeader[2]);
			starttable();
			echo("<tr><td></td><td>\n");
		}
		break;
	case "summary": // text here was moved to full.htm.
		echo("");
		break;
	case "TOC": // load up the $toc variable. 
		$toc = getTOC($PHP_SELF);
		echo ("<h2>$pagetitle</h2>");
	}
}

$isLoaded = "loaded";


