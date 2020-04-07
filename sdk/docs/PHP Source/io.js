// wm.js

function SwitchView(vWhich){
	var FS, bParent=false;

		// open frameset
		//get frameset
		var szLocation=self.location.href;
		FS=GetFrameSet(szLocation);
		vSelf=GetSelf();	
		if (FS=='d')
			{
		//	alert('whichview parent true');
			bParent=true;	//indicates we need to move up a level
			}

		//alert("FS: " + FS + "\nvSelf: " + vSelf);
	if (vWhich==1){	
		LoadFrameSet(FS,vSelf,bParent);
	}else{
		// open page
		if (bParent)
		{
			vSelf = '../' + vSelf;
		}
		//alert(vSelf);
		top.location.href=vSelf;
	}
}
function OpenView(vDest){
	var FS, TS, bParent = false;
	var vSelf;
	// used to open linked files
		var vSelf=GetSelf();
		FS=GetFrameSet(vSelf);
		if (FS == 'd')
			{
			bParent = true;
			}

		TS=GetFrameSet(vDest);
		if (top.frames.length>1){	
		//alert("TS: " + TS + "\nFS: " + FS);
		if (FS==TS){
			self.location.href=vDest;
		}else{
			LoadFrameSet(TS,vDest,bParent);
		}
	}else{
		if (bParent)
			{
			vDest = '../' + vDest;
			}
		top.location.href=vDest;
	}
}

function ShowCopyright(){
	var url='copyright.htm';
	var self=GetSelf();
	if (self.lastIndexOf('/')!=-1) {
		url='../' + url;
		}

	NewWindow(url);
}
function NewWindow(vSelf){
	// get user screen size
	// calculate 50%
	// open a new bare window
	var h=eval(screen.height/2);
	var w=eval(screen.width/2);
	var vLeft=eval((screen.width-w)/2);
	var vTop=eval((screen.height-h)/2);
//	var vFeatures="outerHeight=" + h +", outerWidth=" + w ;
	var vFeatures="resizable=1,scrollbars=1,titlebar=1,width=" + w +", height=" + h + ",screenX=" + vLeft + ", screenY=" + vTop ;
//	alert(vFeatures);
	window.open(vSelf,"Examples",vFeatures);
//	window.open(vSelf,"Examples");
}

function GetFrameSet(szLocation){

	var id=szLocation.lastIndexOf('/');
	if (szLocation.lastIndexOf('doxy/')!=-1) {
		id=szLocation.lastIndexOf('doxy/')-1;
		//alert('doxy');
		}
	var FS;
	if (id){
		FS=szLocation.substring(id+1,id+2);
	}else{
		FS=szLocation.substring(id,id+1);
	}
//alert('GetFrameSet(' + szLocation + ') = ' + FS);
	return FS;
}
function GetSelf(){

	var szLocation=self.location.href;

	var id=szLocation.lastIndexOf('/');
	if (szLocation.lastIndexOf('doxy/')!=-1) {
		id=szLocation.lastIndexOf('doxy/')-1;
		}
	var vSelf;
	if (id){
		vSelf=szLocation.substring(id+1);
	}
//alert('GetSelf(' + szLocation + ') = '+vSelf);		
	return vSelf;
}


function LoadFrameSet(vWhich,vSelf,bParent){
	var vHref;
	switch (vWhich){
		case 'a':
			// architecture
			vHref="archOverview"
			break;
		case 'c':
			// component
			vHref="com";
			break;
		case 'o':
			//intro
			vHref="obj";
			break;
		case 'h':
			// hardware
			vHref="hw";
			break;
		case 'd':
			// Doxygen output
			vHref="doxy";
			//alert('Doxy Load FS');
			break;
	}
	//alert(bParent);
	if (bParent)
		{
		//alert('bParent is True');
		vHref='../' + vHref;
		}
	//alert(vHref + "_FS.html?main=" + vSelf);
	/*	if (top.frames.length>1){ // in this case, a frameset already exists
		parent.location.href=vHref + "_FS.htm?main=" + vSelf; //so just change the TOC
		}
		else {
	top.location.href="index.htm?FS=" + vHref + "_FS.htm&main=" + vSelf;
	}*/
		top.location.href=vHref + "_FS.htm?main=" + vSelf
}

