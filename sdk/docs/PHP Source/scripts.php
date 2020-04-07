<?php
// following function getNav takes 3 arguments: a path or filename,
// and pointers to two arrays. It returns a third array.
// Each array has up to 3 elements. The first element [0] is a filename
// or, if it begins with "#", a bookmark reference.
// The second element [1] is a text label. The third element [2]
// is a header level, 1-3.
// These values are read out of the file "filelist.txt".
//
// This file has each reference on a separate line. First,
// a filename is listed by itself, that refers to a chapter TOC.
// Nothing else should be on a TOC line.
// Blank lines are ignored.
// Chapter TOCs are assembled from all lines containing tabs following 
// the file name of the chapter TOC.

// The getNav function returns the filename, text header, and level of the current file.
// It returns the filename, text header, and level of the next file in the second argument,
// and the filename, text header, and level of the previous file in the third argument.
// It ignores lines beginning with "#", which represent bookmarks.
// It expects the filename, text header, and level to be tab-delimited, 
// and on a single line.

// New on 10/16/2001: now this function also ignores lines that contain ".pdf",
// to omit PDF files from the navigation. They will still appear in the TOC.

function getNav($thisURL, &$nextLine, &$prevLine) 
{
	global $href;
	$tocname = $href."filelist.txt";
	$thisFile = strrpos($thisURL,"/")? substr($thisURL,strrpos($thisURL, "/")+1): $thisURL;
//	echo ($thisFile);

		{
		//$tocname = "/var/www/html/iobjects/" . $tocname;
			if (!file_exists($tocname))
			{
			die("Can't find $tocname!");
			}
		}
	
	$fp = fopen($tocname, "r", 1) or
		die("Can't open $tocname");	
	while ($curLine = fgets($fp, 2048))
	{
		if (ereg("(^|/)".$thisFile, $curLine))
		{
			break;
		}
		
		if (count(explode("\t",$curLine))>1 && !strstr($curLine,"#") && !stristr($curLine,".pdf"))
		{
			$prevLine=explode("\t",$curLine);
		}
		
	}

	while (!feof($fp))
		{
		$next = fgets($fp,2048);
		//echo ($next . "<BR>");
		if (count(explode("\t",$next))>1 && !strstr($next,"#") && !stristr($next,".pdf"))
			{
			$nextLine = explode("\t",$next);
			break;
			}
		}
	fclose($fp);
//	echo ("curLine = $curLine, thisFile = $thisFile<BR>");
	return explode("\t",$curLine);
}

// The following function getTOC()
// takes a filename or URL as an argument
// and returns an array of lines immediately below the TOC
// filename in the "filelist.txt" file. 
// If a given line begins with "#" (indicating a bookmark), 
// this function pre-pends the previous filename to the beginning of
// that array element.
// Each array element contains a string with the filename & bookmark,
// text header, and level, delimited by tabs.

//new on 10/16/2001: This routine now ignores .pdf files when adding a bookmark 
// or crossref link.
function getTOC($thisURL)
{
	global $lasttime;
	$lasttime=0;
	$tocname = "filelist.txt";
	$thisFile = substr($thisURL,strrpos($thisURL, "/")+1);
	//echo ($thisFile."<br>");
	if (!file_exists($tocname))
		{
		die("Can't find $tocname!");
		}
	
	$fp = fopen($tocname, "r", 1) or
		die("Can't open $tocname");	
	
	while ($temp=fgets($fp,2048))
	{
	//echo ("temp=$temp, thisfile=$thisFile<BR>");
	if ($thisFile==trim($temp)) 
		{//echo ("yes!");
		break; }
	}
	if (feof($fp)) { die("Not Found!!!"); }
	while ($temp = fgets($fp, 2048))
	{
	if (trim($temp) =="")
		{ break; }
		switch (substr($temp,0,1))
		{
		case "#": // bookmark; add last filename to bookmark.
			$list[] = $baseFile.$temp;
			break;
		case "%": // hidden link; don't add
			break;
		default: // Filename; store in $basefile and add.
			if (!stristr($temp,".pdf")) //if not .pdf, save 
										//filename and check timestamp.
			{
				$baseFile = substr($temp,0,strpos($temp,"\t"));
				$tmptime = filemtime($baseFile);
	
				$ins = strpos($temp, "\t"); //insert bookmark location
				$temp = substr($temp, 0, $ins) . '#' . substr($temp,0, strpos($temp, '.')) . substr($temp,$ins);
				if ($tmptime>$lasttime)
					{
					$lasttime = $tmptime;
					}
			}
			else 
			{
			$ins = strrpos($temp, "\t");
			$temp = substr($temp,0, $ins) . " (PDF)" . substr($temp,$ins);
			}
			$list[] = $temp;
		}
	}
	fclose($fp);
	return $list;
}

// The formatTOC() function takes a tab-delimited string,
// which would be a single array element from the array 
// returned by the getTOC() function, and formats it as a list
// item and link to the correct entry.
function formatTOC($input)
{
	$header = explode("\t", $input);
	$output = "<li class='toc".trim($header[2])."'><a href='$header[0]' target='main' class='toc".trim($header[2])."'>$header[1]</a></li><br>
";
	return $output;
}

// The showTOC() function prints each element in the $toc array.
function showTOC($toc)

{
	global $lasttime;
	while (list(,$value) = each($toc))
		{
		echo formatTOC($value);
		}
	echo ("<p>&nbsp;</p>");
	global $SCRIPT_NAME;
	$url = strstr($SCRIPT_NAME,"/") ? substr($SCRIPT_NAME,strrpos($SCRIPT_NAME,"/")+1) : $SCRIPT_NAME;
	$file = substr($url, 0, strpos($url, "_"));
	
	/*if (file_exists($file.".doc"))
		{
		echo ("<p><a href=\"$file.doc\" target='_blank'>$file Word Chapter</a>");
		if ($lasttime > filemtime($file.".doc"))
			{ echo " (not current)</p>";
			}
			else
			{ echo " (current)</p>";
			}

		
		}*/
	if (file_exists($file.".pdf") and ($lasttime <= filemtime($file.".pdf")))
		{
		echo ("<p><a href=\"$file.pdf\" target=\"_blank\">Printable Chapter (PDF)</a>");
		/*)
			{ echo " (not current)</p>";
			}
			else
			{ echo " (current)</p>";
			}*/
		}
	/*else
		{
		echo "$file not present";
		
	echo ("<p><a href=\"full.htm?chapter=$url\" target='main'>Full chapter</a></p>");
	}*/
}

// The getFileList() function returns an array containing all of
// the unique filenames in the "filelist.txt" file, in sequence.
function getFileList($chapter="")
{
	$tocname = "filelist.txt";
	if (!file_exists($tocname))
		{
		die("Can't find $tocname!");
		}
	
	$fp = fopen($tocname, "r", 1) or
		die("Can't open $tocname");	
	if (empty($chapter))
	{
		while ($temp=fgets($fp,2048))
		{
		//echo("temp=$temp, thisfile=$thisFile<BR>");
		if (strstr($temp,"\t") && !strstr($temp,"#") && !stristr($temp,".pdf")) 
			{//echo ("yes!");
			$list[] = substr($temp,0,strpos($temp,"\t")); 
			}
		}
	}
	else
	{
		while ($temp=fgets($fp,2048))
		{
			if (strstr($temp,$chapter))
			{
			break;
			}
		}
		if (feof($fp)) {die ("TOC $chapter not found"); }
		while ($temp=fgets($fp, 2048) and (trim($temp)!=""))
		{
			if (strstr($temp,"\t") && !strstr($temp,"#") && !stristr($temp,".pdf"))
				{//echo ("yes!");
				$list[] = substr($temp,0,strpos($temp,"\t")); 
				}
		}
		
	}
		//print_r($list);
	fclose($fp);
	return $list;


}


// the nextLink() and previousLink() functions take the array values
// formatted as the return of the getNav() function above, and creates
// a link. You must pass the $current function in, to determine whether the
// target link is under the same TOC, or a different one, as the target link.
function nextLink($current, $next)
{
	global $href;
	$url = getLink($next[0],$current);
	$output = (empty($url)) ? "" :"<a href=\"$url\" title='$next[1]'><img src='".$href."images/next.gif' width=60 height=10 border='0'></a>";
	return $output;

}

function previousLink($current, $previous)
{
	global $href;
	$url = getLink($previous[0],$current);
	$output = (empty($url)) ? "" : "<a href=\"$url\" title='$previous[1]'><img src='".$href."images/previous.gif' width=78 height=10 border='0'></a>";
	return $output;

}

// the getLink function compares the first letter of the filename of
// the current page to the target link. If they match, they are under
// the same TOC, and so the link is a simple relative link.
// If they do not match, the JavaScript function OpenView() must be
// called to change the frameset.
function getLink($link, $current)
{
//echo $link;
	//global $href;
	 
	$sw = GetFrameSet($link);
	$href = ($sw == "d") ?  "../": "";
	switch ($sw)
	{
	case "":
		//echo ("no match");

		return;
	case "#":
		return $link;
	case GetFrameSet($current):
		$url = $href.$link;
//echo("match");
		break;
	default:
//	echo("default");
		$url = "JavaScript:OpenView('$link');";
	}
	return $url;
}

// this function extracts the first letter of the filename. If the 
// filename is a path, it extracts the first letter following the last
// forward slash.
function GetFrameSet($url){

	$thisFile = strstr($url, "/")? substr($url,strrpos($url, "/")+1,1) 
	: substr($url, 0,1);
	if (strstr($url, "doxy/"))
		{
		$thisFile = "d";
		}
//echo $thisFile;
	return $thisFile;
}

// this function allows for custom handling of different header levels.
// Arguments are for following:
// $name is a name to use as a target for links to this header. No spaces allowed.
// $text is the text to appear to the end user.
// $level is an integer that represents the header level.
// Currently, levels 3 and 4 are placed in a left column of a two
// column table. This may be made conditional on the type of page.
function head($name, $text, $level)
{
	global $tableon, $type;
	if ($level == 0) //let's do a Proc Head style & return
	
		{
		return "<p class='ProcHead'><a name='$name'>$text</a></p>";
		}
	
	endtablerow();
	$label = "<h$level><a name='$name'>$text</a></h$level>";
	if ($tableon)
	{
		switch ($level)
		{
		case 1:
		case 2:	
			$label = "<tr><td colspan=2>$label</td></tr>
				<tr><td width=50>&nbsp;</td><td>";
			break;
		case 3:
		case 4:
		
			$label = "<tr valign='top'><td>$label</td>
				<td>";
		}
	}
	return $label;
}

// This function takes a link URL and the text to show.
// In a page type=summary, the link is to the bookmark reference.
// In other page types, it searches the filelist.txt file to determine
// which page the link is on. If it's on the current page, bookmark link.
// If it's on a different page, jump to correct page & bookmark.
// Filelist.txt file lines beginning with "%" are used to find targets
// without showing in the TOC.
function crossref($link, $text)
{
	global $type, $PHP_SELF;

	if (strstr($link,"http://"))
	{
		return "<a href=\"$link\" target=\"_blank\">$text</a>";
	}

	$filename = "filelist.txt";
	$thisFile = substr($PHP_SELF,strrpos($PHP_SELF, "/")+1);
	//echo ($thisFile."<br>");
	if (!file_exists($filename))
		{
		die("Can't find $tocname!");
		}
	
	$fp = fopen($filename, "r", 1) or
		die("Can't open $tocname");	
	
	while ($temp=fgets($fp,2048))
	{
	//echo ("temp=$temp, thisfile=$thisFile<BR>");
		if (strstr($temp, $link))
			{
			break;
			}
		switch (substr($temp,0,1))
			{
			case "#": // skip
			case "%": // skip
				break;
			default: // Filename; store in $basefile and add.
				$baseFile = substr($temp,0,strpos($temp,"\t"));
			}
	} // if this works, $basefile contains last file, $temp contains matched line.
	
	if (feof($fp)) { die("Link Not Found!!!"); }
	fclose($fp);
	
	switch ($type)
	{
		case "summary":
			if (ereg("([#%]|^)([a-zA-Z0-9]*)($|\t|[.a-zA-Z0-9/_-]*)",$temp,$reg))
			{	//print($temp); print_r($reg);
				global $chapter;
				if (empty($chapter)) { $chapter=""; }
				
				if (strstr($temp,"doxy/"))
				{
					$out = "<a href=\"doxy$reg[3]\">$text</a>";
				}
				else
				{
					$out = "<a href=\"#$reg[2]\">$text</a>";
				}
				return $out;
			}
			else 
			{ die ("Link not matched");
			}
		
		default:
			switch (substr($temp,0,1))
			{
				case "%":
					$temp = $baseFile . "#" . substr($temp,1);
					break;
				case "#":
					if ($baseFile == $thisFile)
					{
						$temp = substr($temp,0,strpos($temp,"\t"));
					} 
					else
					{
						$temp = $baseFile . substr($temp,0,strpos($temp,"\t")) ;
					}
					break;
				default:
					$temp = substr($temp,0,strpos($temp,"\t")) . '#' .
					 substr($temp,0,strpos($temp,".")) ;
			}	
					
			$url = getLink($temp,$thisFile);
			$out = "<a href=\"$url\">$text</a>";
					
			
		
	}
	
	return $out;


}

// This function takes a filename and alt text label for an image.
// It checks the width of the image file. If width is > 450, it uses
// the PictureWide style and spans the two table columns of the main table.
// Otherwise, it uses the Picture style.
function picture($name, $text)
{
	global $type, $tableon;
	$imagefile = "images/$name";
	$size = getImageSize($imagefile);
	$wide = ($size[0]>500) ? "wide" : "narrow";
	$output = "<img src='$imagefile' alt='$text' $size[3]>";
	switch ($wide)
	{
		case "wide":
			endtablerow();
			$output = "<p class='PictureWide'>". $output . "</p>";
			if ($tableon)
				{
				echo ("<tr><td colspan=2>$output</td></tr>
				<tr><td></td><td>\n");
				}
			break;
		case "narrow":
			$output = "<p class='Picture'>". $output . "</p>";	
			echo $output;
	}
}

// Generic function for starting the main table. 
// All other table functions must check the $tableon variable
// before inserting/removing tags.
function starttable()
{
	global $tableon;
	if (!$tableon)
		{
		echo ("<table cellpadding=5>\n");
		$tableon = true;
		}
}

// This function may be called at any point.
// It closes a table row if the table is on.
function endtablerow()
{
	global $tableon;
	if ($tableon)
		{
		echo ("</td></tr>\n");
		}
}

function spantablerow()
{
	global $tableon;
	endtablerow();
	if ($tableon)
		{
		echo ("<tr><td colspan=2>\n");
		}
}

function resumetablerow()
{
	global $tableon;
	endtablerow();
	if ($tableon)
		{
		echo ("<tr><td></td><td>\n");
		}
}



// This function may be called at any point to end the current
// table, but should only be called at the very end of the page,
// to keep the columns aligned.
function endtable()
{
	global $tableon;
	if ($tableon)
		{
		echo ("</table>\n");
		$tableon = false;
		}
}

// This function adds a navigation box, used by normal pages
// at the top and bottom of the page. It shows a previous button,
// a change view button, and a next button. If the page is the first
// or the last in the filelist.txt file, the next or the previous
// button does not show up.
function navBox($file,$next,$prev)
{
	global $href;
//echo ("navbox: $file, $next, $prev<BR>");
		echo <<<EOQ
<table width="100%" >
<tr><td align="left" width="33%">
EOQ;
		echo(previousLink($file, $prev));
	
		echo <<<EOQ
</td><td align="center" width="33%">
<SCRIPT LANGUAGE="JavaScript">
   if (parent.frames.length>1){
       document.write('<A HREF="JavaScript:SwitchView(0);"><img src="
EOQ;
	echo ($href."images/pagevw.gif\" width=49 height=10 border=\"0\"></A>');
   }else{
       document.write('<A HREF=\"JavaScript:SwitchView(1);\"><img src=\"" . $href . "images/normalvw.gif\" width=60 height=8 border=\"0\"></A>');
   }
</SCRIPT>
</td><td align=\"right\" width=\"33%\">
");
		echo(nextLink($file, $next));	
		echo <<<EOQ
</td></tr>
</table>
EOQ;

}

?>