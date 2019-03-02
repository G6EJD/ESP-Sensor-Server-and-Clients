void append_page_header(bool refresh_on) {
  webpage  = F("<!DOCTYPE html><html>");
  webpage += F("<head>");
  webpage += F("<title>Sensor Server</title>"); // NOTE: 1em = 16px
  if (AUpdate && refresh_on) webpage += F("<meta http-equiv='refresh' content='30'>"); // 30-sec refresh time, test needed to prevent auto updates repeating some commands
  webpage += F("<meta name='viewport' content='user-scalable=yes,initial-scale=1.0,width=device-width'>");
  webpage += F("<style>");
  webpage += F("body{max-width:1280px;margin:0 auto;font-family:arial;font-size:105%;text-align:center;color:blue;background-color:#F7F2Fd;}");
  webpage += F("ul{list-style-type:none;margin:0.1em;padding:0;border-radius:0.375em;overflow:hidden;background-color:#dcade6;font-size:1em;}");
  webpage += F("li{float:left;border-radius:0.375em;border-right:0.06em solid #bbb;}last-child {border-right:none;font-size:85%}");
  webpage += F("li a{display: block;border-radius:0.375em;padding:0.44em 0.44em;text-decoration:none;font-size:85%}");
  webpage += F("li a:hover{background-color:#EAE3EA;border-radius:0.375em;font-size:85%}");
  webpage += F("section {font-size:0.88em;}");
  webpage += F("h1{color:yellow;border-radius:0.5em;font-size:1em;padding:0.2em 0.2em;background:#558ED5;}");
  webpage += F("h2{color:orange;font-size:1.0em;}");
  webpage += F("h3{color:blue;font-size:0.8em;}");
  webpage += F("table{font-family:arial,sans-serif;font-size:0.9em;border-collapse:collapse;text-align:left;}"); 
  webpage += F("th,td {border:0.06em solid #dddddd;text-align:left;padding:0.3em;border-bottom:0.06em solid #dddddd;font-size:0.9em;}"); 
  webpage += F("tr:nth-child(odd){background-color:#eeeeee;}");
  webpage += F(".rcorners{border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;color:yellow;font-size:75%;width:50%;}");
  webpage += F(".column{float:left;width:50%;height:45%;}");
  webpage += F(".row:after{content:'';display:table;clear:both;}");
  webpage += F("*{box-sizing:border-box;}");
  webpage += F("footer{background-color:#eedfff;text-align:center;padding:0.3em 0.3em;border-radius:0.375em;font-size:60%;}");
  webpage += F("button{border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:20%;color:yellow;font-size:130%;}");
  webpage += F(".buttons {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:15%;color:yellow;font-size:80%;}");
  webpage += F("a,p{font-size:0.9em;text-align:left;}");
  webpage += F("progress{height:1em;border-radius:0.3em;background-color:white;color:#558ED5;}");
  webpage += F(".container{position:relative;font-family:Arial;text-align:center;overflow-y:hidden;}");
  webpage += F(".centered{position:absolute;top:67%;left:50%;transform:translate(-50%,-50%);background-color:#558ED5;color:yellow;font-size:95%;}");
  webpage += F("</style></head><body><h1>Sensor Server "); webpage += String(ServerVersion) + "</h1>";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void append_page_footer(bool graph_on){ // Saves repeating many lines of code for HTML page footers
  if (graph_on) {
    webpage += F("<a href='/reverse'><button id = 'prev' class='buttons' style='width:10%'>&lt; "); webpage += String(graph_step)+"</button></a>";
    webpage += F("<a href='/forward'><button id = 'next' class='buttons' style='width:10%'>&gt; "); webpage += String(graph_step)+"</button></a><br><br>";
    if (readingCnt > display_records) {
      webpage += "<progress value='"+String(graph_start+graph_end-display_records)+"' max='"+ String(readingCnt-display_records)+"'></progress>";
    }
    else
    {
      webpage += "<progress value='"+String(graph_start+graph_end)+"' max='"+ String(readingCnt)+"'></progress>";
    }
  }
  webpage += F("<ul>");
  webpage += F("<li><a href='/'>Home</a></li>");
  webpage += F("<li><a href='/AUpdate'>Refresh("); webpage += String((AUpdate?"ON":"OFF"))+")</a></li>";
  webpage += F("<li><a href='/Liveview'>View</a></li>");
  webpage += F("<li><a href='/Iconview'>Locations</a></li>");
  webpage += F("<li><a href='/chart'>Graph</a></li>"); 
  webpage += F("<li><a href='/Csetup'>Setup</a></li></ul><ul>");
  webpage += F("<li><a href='/Cstream'>Stream</a></li>");
  webpage += F("<li><a href='/Cdownload'>Download</a></li>");
  webpage += F("<li><a href='/Odownload'>[Download]</a></li>");
  webpage += F("<li><a href='/Cupload'>Upload</a></li>");
  webpage += F("<li><a href='/Cerase'>Erase</a></li>"); 
  webpage += F("<li><a href='/Oerase'>[Erase]</a></li>"); 
  webpage += F("<li><a href='/SDdir'>Directory</a></li>");
  webpage += F("<li><a href='/Help'>Help</a></li>");
  webpage += F("</ul>");
  webpage += "<footer>&copy;"+String(char(byte(0x40>>1)))+String(char(byte(0x88>>1)))+String(char(byte(0x5c>>1)))+String(char(byte(0x98>>1)))+String(char(byte(0x5c>>1)));
  webpage += String(char((0x84>>1)))+String(char(byte(0xd2>>1)))+String(char(0xe4>>1))+String(char(0xc8>>1))+String(char(byte(0x40>>1)));
  webpage += String(char(byte(0x64/2)))+String(char(byte(0x60>>1)))+String(char(byte(0x62>>1)))+String(char(0x70>>1))+"</footer>";
  webpage += F("</body></html>");
}
