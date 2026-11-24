"""Generate importable Node-RED flow with no hand-edited IDs."""
import json
from pathlib import Path

nodes = []
flow = "fire"

def add(id, type, name, x, y, wires=None, **kw):
    nodes.append(dict(id=id, type=type, z=flow, name=name, x=x, y=y,
                      wires=wires or [[]], **kw))

nodes.append({"id": flow, "type": "tab", "label": "Forest fire fog", "disabled": False, "info": "ESP32 and digital twin share one MQTT schema."})
nodes.append({"id": "broker", "type": "mqtt-broker", "name": "Docker Mosquitto", "broker": "mqtt", "port": "1883", "clientid": "nodered-fog", "autoConnect": True, "usetls": False, "protocolVersion": "4", "keepalive": "60", "cleansession": True})
nodes.append({"id": "ui_tab", "type": "ui_tab", "name": "Fire monitor", "icon": "dashboard", "order": 1})
nodes.append({"id": "ui_sensors", "type": "ui_group", "name": "Sensors", "tab": "ui_tab", "order": 1, "disp": True, "width": "12", "collapse": False})
nodes.append({"id": "ui_control", "type": "ui_group", "name": "Admin controls", "tab": "ui_tab", "order": 2, "disp": True, "width": "12", "collapse": False})

for source, y in [("esp32", 100), ("twin", 180)]:
    add(f"in_{source}", "mqtt in", f"{source} telemetry", 120, y, [["parse"]], topic=f"forestfire/{source}/telemetry", qos="0", datatype="auto", broker="broker", nl=False, rap=True, rh=0, inputs=0)
add("parse", "json", "Parse JSON", 300, 140, [["validate"]], property="payload", action="obj", pretty=False)
validate = r'''const p=msg.payload;
if (!p || !["esp32","twin"].includes(p.source) ||
    !["OFF","SAFE","WARNING","ALARM"].includes(p.state) ||
    typeof p.flame!=="boolean" || typeof p.motion!=="boolean" ||
    (p.temperature_c!==null && typeof p.temperature_c!=="number") ||
    (p.humidity!==null && typeof p.humidity!=="number")) return null;
const s=flow.get("selectedSource") || "twin";
if (p.source!==s) return null;
p.received_at=Date.now();
flow.set("latest",p);
msg.payload=p;
return msg;'''
add("validate", "function", "Validate and select source", 480, 140, [["split", "history"]], func=validate, outputs=1, noerr=0)
add("split", "function", "Dashboard and cloud fields", 670, 110, [["temp", "hum", "flame", "state", "recommend", "cloud"]], outputs=1, noerr=0,
    func=r'''const p=msg.payload;
msg.temperature=p.temperature_c;
msg.humidity=p.humidity;
msg.flame_label=p.flame ? "FLAME DETECTED" : "clear";
msg.state_label=p.state + (p.motion && p.state==="ALARM" ? " - PERSON NEAR" : "");
msg.recommendation=p.state==="ALARM" ? "Alarm active. Inspect site; mute only after checking." :
  p.state==="WARNING" ? "Elevated risk. Check heat and dry conditions." : "Conditions normal. Keep monitoring.";
return msg;''')

for id, label, field, y in [("temp","Temperature C","temperature",70),("hum","Humidity %","humidity",110),("flame","Flame","flame_label",150),("state","State","state_label",190),("recommend","Recommendation","recommendation",230)]:
    nodes.append({"id":id,"type":"ui_text","z":flow,"group":"ui_sensors","order":(y-30)//40,"width":"6" if id in ("temp","hum") else "12","height":"1","name":label,"label":label,"format":"{{msg."+field+"}}","layout":"row-spread","x":900,"y":y,"wires":[]})

add("history", "function", "Store bounded history", 690, 290, [[]], outputs=1, noerr=0,
    func='let h=flow.get("history")||[];h.push(msg.payload);if(h.length>720)h=h.slice(-720);flow.set("history",h);return null;')
add("cloud", "function", "ThingSpeak rate limit", 910, 290, [["http"]], outputs=1, noerr=0,
    func=r'''const p=msg.payload;
const key=env.get("THINGSPEAK_WRITE_KEY");
if (!key) return null;
const now=Date.now();
if (now-(flow.get("lastCloud")||0)<16000) return null;
flow.set("lastCloud",now);
msg.method="POST";msg.url="https://api.thingspeak.com/update.json";
msg.headers={"Content-Type":"application/x-www-form-urlencoded"};
msg.payload="api_key="+encodeURIComponent(key)+
 "&field1="+encodeURIComponent(p.temperature_c??"")+
 "&field2="+encodeURIComponent(p.humidity??"")+
 "&field3="+Number(p.flame)+"&field4="+Number(p.motion)+
 "&field5="+({OFF:0,SAFE:1,WARNING:2,ALARM:3}[p.state])+ 
 "&field6="+p.flame_ao+"&field7="+p.servo_deg+"&field8="+Number(p.muted);
return msg;''')
add("http", "http request", "Public ThingSpeak channel", 1120, 290, [["cloud_debug"]], method="use", ret="obj", paytoqs="ignore", url="", tls="", persist=False, proxy="", insecureHTTPParser=False, authType="", senderr=False, headers=[])
add("cloud_debug", "debug", "Cloud response/errors", 1320, 290, [[]], active=True, tosidebar=True, complete="payload", targetType="msg")

for id, label, command, y in [("mute","Mute buzzer","MUTE",370),("unmute","Unmute buzzer","UNMUTE",410),("teston","Alarm test ON","TEST_ON",450),("testoff","Alarm test OFF","TEST_OFF",490)]:
    nodes.append({"id":id,"type":"ui_button","z":flow,"name":label,"group":"ui_control","order":(y-330)//40,"width":"3","height":"1","passthru":False,"label":label,"tooltip":"","color":"","bgcolor":"","icon":"","payload":command,"payloadType":"str","topic":"","topicType":"str","x":160,"y":y,"wires":[["command_route"]]})
add("command_route", "function", "Route to selected source", 410, 430, [["mqtt_out"]], outputs=1, noerr=0,
    func='msg.topic="forestfire/"+(flow.get("selectedSource")||"twin")+"/command";return msg;')
add("mqtt_out", "mqtt out", "Commands", 650, 430, [], topic="", qos="0", retain="false", broker="broker", inputs=1)
for id,label,source,y in [("select_twin","Use digital twin","twin",540),("select_esp32","Use ESP32","esp32",580)]:
    nodes.append({"id":id,"type":"ui_button","z":flow,"name":label,"group":"ui_control","order":(y-450)//40,"width":"6","height":"1","passthru":False,"label":label,"tooltip":"","color":"","bgcolor":"","icon":"","payload":source,"payloadType":"str","topic":"","topicType":"str","x":170,"y":y,"wires":[["select_source"]]})
add("select_source", "function", "Select monitored source", 430, 560, [[]], outputs=1, noerr=0, func='flow.set("selectedSource",msg.payload);return null;')
nodes.append({"id":"status_text","type":"ui_text","z":flow,"group":"ui_sensors","order":6,"width":"12","height":"1","name":"Connection","label":"Source / freshness","format":"{{msg.payload}}","layout":"row-spread","x":690,"y":650,"wires":[]})
add("tick", "inject", "Every 5 seconds", 140, 650, [["freshness"]], props=[{"p":"payload"}], repeat="5", crontab="", once=True, onceDelay="1", topic="", payload="", payloadType="date")
add("freshness", "function", "Mark stale data", 420, 650, [["status_text"]], outputs=1, noerr=0,
    func='const p=flow.get("latest"),s=flow.get("selectedSource")||"twin";msg.payload=s+" / "+(!p||p.source!==s||Date.now()-p.received_at>15000?"STALE OR OFFLINE":"LIVE");return msg;')

Path("fog/flows.json").write_text(json.dumps(nodes, indent=2)+"\n", encoding="utf-8")
