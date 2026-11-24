"""Create a readable SVG pin-level wiring chart from the firmware pin map."""
from pathlib import Path
from html import escape

W, H = 1600, 1210
parts = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">',
         '<rect width="100%" height="100%" fill="#f8fafc"/>',
         '<style>text{font-family:Arial,sans-serif;fill:#172033}.title{font-size:36px;font-weight:700}.sub{font-size:18px;fill:#526172}.head{font-size:22px;font-weight:700}.pin{font-size:16px}.note{font-size:16px;fill:#526172}.board{fill:#fff;stroke:#314a67;stroke-width:3}.card{fill:#fff;stroke:#b9c8d8;stroke-width:2}.wire{fill:none;stroke-width:3;stroke-linecap:round}</style>',
         '<text x="55" y="58" class="title">Forest Fire IoT - hardware wiring</text>',
         '<text x="55" y="88" class="sub">ESP32 DevKit V1 / DOIT  |  Project 2 pin map  |  separate SPI microSD reader</text>']

def rect(x,y,w,h,cls='card',r=12): parts.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{r}" class="{cls}"/>')
def txt(x,y,s,cls='pin'): parts.append(f'<text x="{x}" y="{y}" class="{cls}">{escape(s)}</text>')
def line(x1,y1,x2,y2,color='#486581'): parts.append(f'<path d="M{x1} {y1} H{x2}" class="wire" stroke="{color}"/>')

rect(580,125,420,930,'board',18)
txt(650,170,'ESP32 DEVKIT V1','head')
txt(650,198,'3V3 / GND / VIN rails','note')
left = [
 ('DHT11', ['VCC → 3V3','DATA → GPIO4','GND → GND']),
 ('KY-026 flame', ['VCC → 3V3','AO → GPIO34','DO → GPIO35 (optional)','GND → GND']),
 ('HC-SR501 PIR', ['VCC → 5V/VIN','OUT → GPIO27','GND → GND']),
 ('Push button', ['contact → GPIO13','other contact → GND']),
 ('Active buzzer', ['+ → GPIO26','- → GND']),
]
right = [
 ('ILI9341 TFT', ['VCC / LED → 3V3','GND → GND','SCK → GPIO18','MOSI → GPIO23','CS → GPIO5','DC → GPIO16','RST → GPIO17']),
 ('microSD reader', ['VCC → 3V3 / GND → GND','SCK → GPIO18','MOSI → GPIO23','MISO → GPIO19','CS → GPIO22']),
 ('RGB LED common cathode', ['R → 220Ω → GPIO25','G → 220Ω → GPIO33','B → 220Ω → GPIO32','common → GND']),
 ('Servo + external 5 V', ['signal → GPIO21','+ → regulated 5 V','- → shared GND']),
]
def cards(items,x,side,start,step):
    for i,(name,pins) in enumerate(items):
        y=start+i*step; h=52+29*len(pins)
        rect(x,y,430,h)
        txt(x+18,y+32,name,'head')
        for j,pin in enumerate(pins): txt(x+20,y+64+j*27,pin)
        cy=y+h/2
        if side=='left': line(x+430,cy,580,cy)
        else: line(1000,cy,x,cy)
cards(left,55,'left',125,180)
cards(right,1100,'right',125,260)
txt(630,290,'GPIO4   DHT11 DATA')
txt(630,340,'GPIO34  flame AO')
txt(630,390,'GPIO35  flame DO')
txt(630,440,'GPIO27  PIR OUT')
txt(630,490,'GPIO13  button')
txt(630,540,'GPIO26  buzzer')
txt(630,590,'GPIO18  SPI SCK (shared)')
txt(630,640,'GPIO23  SPI MOSI (shared)')
txt(630,690,'GPIO19  SD MISO')
txt(630,740,'GPIO5 / 16 / 17  TFT')
txt(630,790,'GPIO22  SD CS')
txt(630,840,'GPIO25 / 33 / 32  RGB')
txt(630,890,'GPIO21  servo signal')
rect(55,1080,1475,96)
txt(78,1118,'Power rule: all grounds must connect. Servo +5 V comes from its own regulated supply, not ESP32 3V3.','head')
txt(78,1150,'TFT and SD share SPI clock/data, but each has its own chip select. PIR was absent from the old Fritzing drawing.','note')
parts.append('</svg>')
Path('docs/hardware_wiring.svg').write_text('\n'.join(parts),encoding='utf-8')
