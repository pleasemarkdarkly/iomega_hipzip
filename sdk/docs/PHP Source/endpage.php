<?php
	endtablerow();
	endtable();
	if ($type=="normal"||$type=="doxy"||empty($type))
	{
	echo "
	<hr><br>";
	navbox($PHP_SELF,$nextPage,$previousPage);
	}

?>
<p class="Legal">
<a href="JavaScript:ShowCopyright();">Copyright &#169; 1998 - 2001</a> Interactive Objects<SUP><FONT SIZE="-3">TM</FONT></SUP>. All rights reserved.
<br>

<?php
//phpinfo();
$last_modified = filemtime($PATH_TRANSLATED);
echo(" Last Modified on ");
echo(date("M j, Y g:i a", $last_modified));
?>

</p>
</body>

</html>
