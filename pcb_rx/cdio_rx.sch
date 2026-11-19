<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE eagle SYSTEM "eagle.dtd">
<eagle version="9.6.2">
<drawing>
<settings>
<setting alwaysvectorfont="no"/>
<setting verticaltext="up"/>
</settings>
<grid distance="0.635" unitdist="mm" unit="mm" style="lines" multiple="1" display="no" altdistance="0.127" altunitdist="mm" altunit="mm"/>
<layers>
<layer number="1" name="Top" color="4" fill="1" visible="yes" active="yes"/>
<layer number="16" name="Bottom" color="1" fill="1" visible="yes" active="yes"/>
<layer number="17" name="Pads" color="2" fill="1" visible="yes" active="yes"/>
<layer number="18" name="Vias" color="2" fill="1" visible="yes" active="yes"/>
<layer number="19" name="Unrouted" color="6" fill="1" visible="yes" active="yes"/>
<layer number="20" name="Dimension" color="24" fill="1" visible="yes" active="yes"/>
<layer number="21" name="tPlace" color="7" fill="1" visible="yes" active="yes"/>
<layer number="22" name="bPlace" color="7" fill="1" visible="yes" active="yes"/>
<layer number="23" name="tOrigins" color="15" fill="1" visible="yes" active="yes"/>
<layer number="24" name="bOrigins" color="15" fill="1" visible="yes" active="yes"/>
<layer number="25" name="tNames" color="7" fill="1" visible="yes" active="yes"/>
<layer number="26" name="bNames" color="7" fill="1" visible="yes" active="yes"/>
<layer number="27" name="tValues" color="7" fill="1" visible="yes" active="yes"/>
<layer number="28" name="bValues" color="7" fill="1" visible="yes" active="yes"/>
<layer number="29" name="tStop" color="7" fill="1" visible="yes" active="yes"/>
<layer number="30" name="bStop" color="7" fill="1" visible="yes" active="yes"/>
<layer number="31" name="tCream" color="7" fill="1" visible="yes" active="yes"/>
<layer number="32" name="bCream" color="7" fill="1" visible="yes" active="yes"/>
<layer number="35" name="tGlue" color="7" fill="1" visible="yes" active="yes"/>
<layer number="36" name="bGlue" color="7" fill="1" visible="yes" active="yes"/>
<layer number="39" name="tKeepout" color="4" fill="1" visible="yes" active="yes"/>
<layer number="40" name="bKeepout" color="1" fill="1" visible="yes" active="yes"/>
<layer number="41" name="tRestrict" color="4" fill="1" visible="yes" active="yes"/>
<layer number="42" name="bRestrict" color="1" fill="1" visible="yes" active="yes"/>
<layer number="43" name="vRestrict" color="2" fill="1" visible="yes" active="yes"/>
<layer number="44" name="Drills" color="7" fill="1" visible="yes" active="yes"/>
<layer number="45" name="Holes" color="7" fill="1" visible="yes" active="yes"/>
<layer number="46" name="Milling" color="3" fill="1" visible="yes" active="yes"/>
<layer number="47" name="Measures" color="7" fill="1" visible="yes" active="yes"/>
<layer number="48" name="Document" color="7" fill="1" visible="yes" active="yes"/>
<layer number="49" name="Reference" color="7" fill="1" visible="yes" active="yes"/>
<layer number="51" name="tDocu" color="7" fill="1" visible="yes" active="yes"/>
<layer number="52" name="bDocu" color="7" fill="1" visible="yes" active="yes"/>
<layer number="88" name="SimResults" color="9" fill="1" visible="yes" active="yes"/>
<layer number="89" name="SimProbes" color="9" fill="1" visible="yes" active="yes"/>
<layer number="90" name="Modules" color="5" fill="1" visible="yes" active="yes"/>
<layer number="91" name="Nets" color="2" fill="1" visible="yes" active="yes"/>
<layer number="92" name="Busses" color="1" fill="1" visible="yes" active="yes"/>
<layer number="93" name="Pins" color="2" fill="1" visible="yes" active="yes"/>
<layer number="94" name="Symbols" color="4" fill="1" visible="yes" active="yes"/>
<layer number="95" name="Names" color="7" fill="1" visible="yes" active="yes"/>
<layer number="96" name="Values" color="7" fill="1" visible="yes" active="yes"/>
<layer number="97" name="Info" color="7" fill="1" visible="yes" active="yes"/>
<layer number="98" name="Guide" color="6" fill="1" visible="yes" active="yes"/>
</layers>
<schematic xreflabel="%F%N/%S.%C%R" xrefpart="/%S.%C%R">
<description>CDIO - PCB del auto (RX). Generado por pcb_rx/generar_pcb.py</description>
<libraries>
<library name="cdio">
<description>Biblioteca del proyecto CDIO</description>
<packages>
<package name="KF301-2">
<description>Bornera de tornillo 2 polos, paso 5,08 mm (KF301-2P)</description>
<pad name="1" x="0" y="0" drill="1.3" diameter="2.8" shape="round"/>
<pad name="2" x="5.08" y="0" drill="1.3" diameter="2.8" shape="round"/>
<wire x1="-2.54" y1="-4.1" x2="7.62" y2="-4.1" width="0.2" layer="21"/>
<wire x1="7.62" y1="-4.1" x2="7.62" y2="3.5" width="0.2" layer="21"/>
<wire x1="7.62" y1="3.5" x2="-2.54" y2="3.5" width="0.2" layer="21"/>
<wire x1="-2.54" y1="3.5" x2="-2.54" y2="-4.1" width="0.2" layer="21"/>
<wire x1="-2.54" y1="-2.8" x2="7.62" y2="-2.8" width="0.2" layer="21"/>
<text x="0" y="4.2" size="1.2" layer="25">&gt;NAME</text>
<text x="0" y="-5.6" size="1" layer="27">&gt;VALUE</text>
</package>
<package name="KF301-3">
<description>Bornera de tornillo 3 polos, paso 5,08 mm (KF301-3P)</description>
<pad name="1" x="0" y="0" drill="1.3" diameter="2.8" shape="round"/>
<pad name="2" x="5.08" y="0" drill="1.3" diameter="2.8" shape="round"/>
<pad name="3" x="10.16" y="0" drill="1.3" diameter="2.8" shape="round"/>
<wire x1="-2.54" y1="-4.1" x2="12.7" y2="-4.1" width="0.2" layer="21"/>
<wire x1="12.7" y1="-4.1" x2="12.7" y2="3.5" width="0.2" layer="21"/>
<wire x1="12.7" y1="3.5" x2="-2.54" y2="3.5" width="0.2" layer="21"/>
<wire x1="-2.54" y1="3.5" x2="-2.54" y2="-4.1" width="0.2" layer="21"/>
<wire x1="-2.54" y1="-2.8" x2="12.7" y2="-2.8" width="0.2" layer="21"/>
<text x="0" y="4.2" size="1.2" layer="25">&gt;NAME</text>
<text x="0" y="-5.6" size="1" layer="27">&gt;VALUE</text>
</package>
<package name="HDR1X3">
<description>Tira de 3 pines macho 2,54 mm (conector del servo)</description>
<pad name="1" x="0" y="0" drill="1.0" diameter="1.9" shape="long" rot="R90"/>
<pad name="2" x="2.54" y="0" drill="1.0" diameter="1.9" shape="long" rot="R90"/>
<pad name="3" x="5.08" y="0" drill="1.0" diameter="1.9" shape="long" rot="R90"/>
<wire x1="-1.27" y1="-2.2" x2="6.35" y2="-2.2" width="0.2" layer="21"/>
<wire x1="6.35" y1="-2.2" x2="6.35" y2="2.2" width="0.2" layer="21"/>
<wire x1="6.35" y1="2.2" x2="-1.27" y2="2.2" width="0.2" layer="21"/>
<wire x1="-1.27" y1="2.2" x2="-1.27" y2="-2.2" width="0.2" layer="21"/>
<text x="0" y="4.2" size="1.2" layer="25">&gt;NAME</text>
<text x="0" y="-5.6" size="1" layer="27">&gt;VALUE</text>
</package>
<package name="R-0207/5V">
<description>Resistencia 1/4 W de pie, paso 5,08 mm</description>
<pad name="1" x="0" y="0" drill="1.0" diameter="2.4" shape="round"/>
<pad name="2" x="5.08" y="0" drill="1.0" diameter="2.4" shape="round"/>
<circle x="0" y="0" radius="1.6" width="0.2" layer="21"/>
<text x="0" y="4.2" size="1.2" layer="25">&gt;NAME</text>
<text x="0" y="-5.6" size="1" layer="27">&gt;VALUE</text>
</package>
<package name="R-0207/10">
<description>Resistencia 1/4 W acostada, paso 10,16 mm</description>
<pad name="1" x="0" y="0" drill="1.0" diameter="2.4" shape="round"/>
<pad name="2" x="10.16" y="0" drill="1.0" diameter="2.4" shape="round"/>
<wire x1="2.2" y1="-1.2" x2="7.96" y2="-1.2" width="0.2" layer="21"/>
<wire x1="7.96" y1="-1.2" x2="7.96" y2="1.2" width="0.2" layer="21"/>
<wire x1="7.96" y1="1.2" x2="2.2" y2="1.2" width="0.2" layer="21"/>
<wire x1="2.2" y1="1.2" x2="2.2" y2="-1.2" width="0.2" layer="21"/>
<text x="0" y="4.2" size="1.2" layer="25">&gt;NAME</text>
<text x="0" y="-5.6" size="1" layer="27">&gt;VALUE</text>
</package>
<package name="R-0207/15">
<description>Resistencia 1/4 W acostada, paso 15,24 mm (pasa por encima de pistas)</description>
<pad name="1" x="0" y="0" drill="1.0" diameter="2.4" shape="round"/>
<pad name="2" x="15.24" y="0" drill="1.0" diameter="2.4" shape="round"/>
<wire x1="4.5" y1="-1.2" x2="10.74" y2="-1.2" width="0.2" layer="21"/>
<wire x1="10.74" y1="-1.2" x2="10.74" y2="1.2" width="0.2" layer="21"/>
<wire x1="10.74" y1="1.2" x2="4.5" y2="1.2" width="0.2" layer="21"/>
<wire x1="4.5" y1="1.2" x2="4.5" y2="-1.2" width="0.2" layer="21"/>
<text x="0" y="4.2" size="1.2" layer="25">&gt;NAME</text>
<text x="0" y="-5.6" size="1" layer="27">&gt;VALUE</text>
</package>
<package name="C-5">
<description>Condensador cerámico, paso 5,08 mm</description>
<pad name="1" x="0" y="0" drill="1.0" diameter="2.4" shape="round"/>
<pad name="2" x="5.08" y="0" drill="1.0" diameter="2.4" shape="round"/>
<wire x1="1" y1="-1.5" x2="4.08" y2="-1.5" width="0.2" layer="21"/>
<wire x1="4.08" y1="-1.5" x2="4.08" y2="1.5" width="0.2" layer="21"/>
<wire x1="4.08" y1="1.5" x2="1" y2="1.5" width="0.2" layer="21"/>
<wire x1="1" y1="1.5" x2="1" y2="-1.5" width="0.2" layer="21"/>
<text x="0" y="4.2" size="1.2" layer="25">&gt;NAME</text>
<text x="0" y="-5.6" size="1" layer="27">&gt;VALUE</text>
</package>
<package name="CPOL-5-D8">
<description>Electrolítico Ø8 mm, paso 5,08 mm (pad 1 = +)</description>
<pad name="1" x="0" y="0" drill="1.0" diameter="2.4" shape="round"/>
<pad name="2" x="5.08" y="0" drill="1.0" diameter="2.4" shape="round"/>
<circle x="2.54" y="0" radius="4" width="0.2" layer="21"/>
<text x="-1.6" y="1.4" size="1.2" layer="21">+</text>
<text x="0" y="4.2" size="1.2" layer="25">&gt;NAME</text>
<text x="0" y="-5.6" size="1" layer="27">&gt;VALUE</text>
</package>
<package name="CPOL-5-D10">
<description>Electrolítico Ø10 mm, paso 5,08 mm (pad 1 = +)</description>
<pad name="1" x="0" y="0" drill="1.0" diameter="2.4" shape="round"/>
<pad name="2" x="5.08" y="0" drill="1.0" diameter="2.4" shape="round"/>
<circle x="2.54" y="0" radius="5" width="0.2" layer="21"/>
<text x="-1.6" y="1.4" size="1.2" layer="21">+</text>
<text x="0" y="4.2" size="1.2" layer="25">&gt;NAME</text>
<text x="0" y="-5.6" size="1" layer="27">&gt;VALUE</text>
</package>
</packages>
<symbols>
<symbol name="CON2">
<wire x1="-2.54" y1="2.54" x2="5.08" y2="2.54" width="0.254" layer="94"/>
<wire x1="5.08" y1="2.54" x2="5.08" y2="-5.08" width="0.254" layer="94"/>
<wire x1="5.08" y1="-5.08" x2="-2.54" y2="-5.08" width="0.254" layer="94"/>
<wire x1="-2.54" y1="-5.08" x2="-2.54" y2="2.54" width="0.254" layer="94"/>
<text x="-2.54" y="3.302" size="1.778" layer="95">&gt;NAME</text>
<text x="-2.54" y="-7.62" size="1.778" layer="96">&gt;VALUE</text>
<pin name="1" x="-5.08" y="0" visible="pad" length="short" direction="pas" rot="R0"/>
<pin name="2" x="-5.08" y="-2.54" visible="pad" length="short" direction="pas" rot="R0"/>
</symbol>
<symbol name="CON3">
<wire x1="-2.54" y1="2.54" x2="5.08" y2="2.54" width="0.254" layer="94"/>
<wire x1="5.08" y1="2.54" x2="5.08" y2="-7.62" width="0.254" layer="94"/>
<wire x1="5.08" y1="-7.62" x2="-2.54" y2="-7.62" width="0.254" layer="94"/>
<wire x1="-2.54" y1="-7.62" x2="-2.54" y2="2.54" width="0.254" layer="94"/>
<text x="-2.54" y="3.302" size="1.778" layer="95">&gt;NAME</text>
<text x="-2.54" y="-10.16" size="1.778" layer="96">&gt;VALUE</text>
<pin name="1" x="-5.08" y="0" visible="pad" length="short" direction="pas" rot="R0"/>
<pin name="2" x="-5.08" y="-2.54" visible="pad" length="short" direction="pas" rot="R0"/>
<pin name="3" x="-5.08" y="-5.08" visible="pad" length="short" direction="pas" rot="R0"/>
</symbol>
<symbol name="RES">
<wire x1="-2.54" y1="1.016" x2="2.54" y2="1.016" width="0.254" layer="94"/>
<wire x1="2.54" y1="1.016" x2="2.54" y2="-1.016" width="0.254" layer="94"/>
<wire x1="2.54" y1="-1.016" x2="-2.54" y2="-1.016" width="0.254" layer="94"/>
<wire x1="-2.54" y1="-1.016" x2="-2.54" y2="1.016" width="0.254" layer="94"/>
<text x="-2.54" y="1.778" size="1.778" layer="95">&gt;NAME</text>
<text x="-2.54" y="-3.556" size="1.778" layer="96">&gt;VALUE</text>
<pin name="1" x="-5.08" y="0" visible="pad" length="short" direction="pas" rot="R0"/>
<pin name="2" x="5.08" y="0" visible="pad" length="short" direction="pas" rot="R180"/>
</symbol>
<symbol name="CAP">
<wire x1="-2.54" y1="1.016" x2="2.54" y2="1.016" width="0.254" layer="94"/>
<wire x1="2.54" y1="1.016" x2="2.54" y2="-1.016" width="0.254" layer="94"/>
<wire x1="2.54" y1="-1.016" x2="-2.54" y2="-1.016" width="0.254" layer="94"/>
<wire x1="-2.54" y1="-1.016" x2="-2.54" y2="1.016" width="0.254" layer="94"/>
<text x="-2.54" y="1.778" size="1.778" layer="95">&gt;NAME</text>
<text x="-2.54" y="-3.556" size="1.778" layer="96">&gt;VALUE</text>
<pin name="1" x="-5.08" y="0" visible="pad" length="short" direction="pas" rot="R0"/>
<pin name="2" x="5.08" y="0" visible="pad" length="short" direction="pas" rot="R180"/>
</symbol>
<symbol name="CPOL">
<wire x1="-2.54" y1="1.016" x2="2.54" y2="1.016" width="0.254" layer="94"/>
<wire x1="2.54" y1="1.016" x2="2.54" y2="-1.016" width="0.254" layer="94"/>
<wire x1="2.54" y1="-1.016" x2="-2.54" y2="-1.016" width="0.254" layer="94"/>
<wire x1="-2.54" y1="-1.016" x2="-2.54" y2="1.016" width="0.254" layer="94"/>
<text x="-2.54" y="1.778" size="1.778" layer="95">&gt;NAME</text>
<text x="-2.54" y="-3.556" size="1.778" layer="96">&gt;VALUE</text>
<pin name="+" x="-5.08" y="0" visible="pad" length="short" direction="pas" rot="R0"/>
<pin name="-" x="5.08" y="0" visible="pad" length="short" direction="pas" rot="R180"/>
</symbol>
</symbols>
<devicesets>
<deviceset name="TB2" prefix="J">
<gates>
<gate name="G$1" symbol="CON2" x="0" y="0"/>
</gates>
<devices>
<device name="" package="KF301-2">
<connects>
<connect gate="G$1" pin="1" pad="1"/>
<connect gate="G$1" pin="2" pad="2"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="TB3" prefix="J">
<gates>
<gate name="G$1" symbol="CON3" x="0" y="0"/>
</gates>
<devices>
<device name="" package="KF301-3">
<connects>
<connect gate="G$1" pin="1" pad="1"/>
<connect gate="G$1" pin="2" pad="2"/>
<connect gate="G$1" pin="3" pad="3"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="SERVO" prefix="J">
<gates>
<gate name="G$1" symbol="CON3" x="0" y="0"/>
</gates>
<devices>
<device name="" package="HDR1X3">
<connects>
<connect gate="G$1" pin="1" pad="1"/>
<connect gate="G$1" pin="2" pad="2"/>
<connect gate="G$1" pin="3" pad="3"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="R5" prefix="R" uservalue="yes">
<gates>
<gate name="G$1" symbol="RES" x="0" y="0"/>
</gates>
<devices>
<device name="" package="R-0207/5V">
<connects>
<connect gate="G$1" pin="1" pad="1"/>
<connect gate="G$1" pin="2" pad="2"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="R10" prefix="R" uservalue="yes">
<gates>
<gate name="G$1" symbol="RES" x="0" y="0"/>
</gates>
<devices>
<device name="" package="R-0207/10">
<connects>
<connect gate="G$1" pin="1" pad="1"/>
<connect gate="G$1" pin="2" pad="2"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="R15" prefix="R" uservalue="yes">
<gates>
<gate name="G$1" symbol="RES" x="0" y="0"/>
</gates>
<devices>
<device name="" package="R-0207/15">
<connects>
<connect gate="G$1" pin="1" pad="1"/>
<connect gate="G$1" pin="2" pad="2"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="C5" prefix="C" uservalue="yes">
<gates>
<gate name="G$1" symbol="CAP" x="0" y="0"/>
</gates>
<devices>
<device name="" package="C-5">
<connects>
<connect gate="G$1" pin="1" pad="1"/>
<connect gate="G$1" pin="2" pad="2"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="CP8" prefix="C" uservalue="yes">
<gates>
<gate name="G$1" symbol="CPOL" x="0" y="0"/>
</gates>
<devices>
<device name="" package="CPOL-5-D8">
<connects>
<connect gate="G$1" pin="+" pad="1"/>
<connect gate="G$1" pin="-" pad="2"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="CP10" prefix="C" uservalue="yes">
<gates>
<gate name="G$1" symbol="CPOL" x="0" y="0"/>
</gates>
<devices>
<device name="" package="CPOL-5-D10">
<connects>
<connect gate="G$1" pin="+" pad="1"/>
<connect gate="G$1" pin="-" pad="2"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
</devicesets>
</library>
</libraries>
<attributes>
</attributes>
<variantdefs>
</variantdefs>
<classes>
<class number="0" name="default" width="0" drill="0">
</class>
</classes>
<parts>
<part name="J1" library="cdio" deviceset="TB2" device="" value="BATERIA"/>
<part name="J2" library="cdio" deviceset="TB2" device="" value="LM2596_IN"/>
<part name="J3" library="cdio" deviceset="TB2" device="" value="LM2596_OUT"/>
<part name="J5" library="cdio" deviceset="TB3" device="" value="ESP32_MOTOR"/>
<part name="J6" library="cdio" deviceset="TB2" device="" value="ESP32_SERVO_BAT"/>
<part name="J4" library="cdio" deviceset="TB2" device="" value="ESP32_POWER"/>
<part name="J7" library="cdio" deviceset="TB3" device="" value="BTS7960_A"/>
<part name="J8" library="cdio" deviceset="TB3" device="" value="BTS7960_B"/>
<part name="J9" library="cdio" deviceset="SERVO" device="" value="SERVO"/>
<part name="R1" library="cdio" deviceset="R15" device="" value="56k"/>
<part name="R2" library="cdio" deviceset="R5" device="" value="10k"/>
<part name="C1" library="cdio" deviceset="C5" device="" value="100nF"/>
<part name="R3" library="cdio" deviceset="R15" device="" value="10k"/>
<part name="R4" library="cdio" deviceset="R10" device="" value="10k"/>
<part name="R5" library="cdio" deviceset="R5" device="" value="10k"/>
<part name="C3" library="cdio" deviceset="CP8" device="" value="100uF"/>
<part name="C2" library="cdio" deviceset="CP10" device="" value="1000uF"/>
</parts>
<sheets>
<sheet>
<plain>
<text x="30.48" y="182.88" size="3.81" layer="97">CDIO - Auto (RX): hub de conexiones</text>
</plain>
<instances>
<instance part="J1" gate="G$1" x="30.48" y="160.02"/>
<instance part="J2" gate="G$1" x="30.48" y="144.78"/>
<instance part="J3" gate="G$1" x="30.48" y="129.54"/>
<instance part="J5" gate="G$1" x="30.48" y="96.52"/>
<instance part="J6" gate="G$1" x="30.48" y="76.2"/>
<instance part="J4" gate="G$1" x="30.48" y="114.3"/>
<instance part="J7" gate="G$1" x="106.68" y="160.02"/>
<instance part="J8" gate="G$1" x="106.68" y="137.16"/>
<instance part="J9" gate="G$1" x="106.68" y="114.3"/>
<instance part="R1" gate="G$1" x="101.6" y="88.9"/>
<instance part="R2" gate="G$1" x="101.6" y="76.2"/>
<instance part="C1" gate="G$1" x="101.6" y="63.5"/>
<instance part="R3" gate="G$1" x="160.02" y="160.02"/>
<instance part="R4" gate="G$1" x="160.02" y="147.32"/>
<instance part="R5" gate="G$1" x="160.02" y="134.62"/>
<instance part="C3" gate="G$1" x="160.02" y="101.6"/>
<instance part="C2" gate="G$1" x="160.02" y="114.3"/>
</instances>
<busses>
</busses>
<nets>
<net name="5V" class="0">
<segment>
<pinref part="J3" gate="G$1" pin="1"/>
<wire x1="25.4" y1="129.54" x2="20.32" y2="129.54" width="0.1524" layer="91"/>
<label x="20.32" y="129.54" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="J4" gate="G$1" pin="1"/>
<wire x1="25.4" y1="114.3" x2="20.32" y2="114.3" width="0.1524" layer="91"/>
<label x="20.32" y="114.3" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="J8" gate="G$1" pin="2"/>
<wire x1="101.6" y1="134.62" x2="96.52" y2="134.62" width="0.1524" layer="91"/>
<label x="96.52" y="134.62" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="J9" gate="G$1" pin="2"/>
<wire x1="101.6" y1="111.76" x2="96.52" y2="111.76" width="0.1524" layer="91"/>
<label x="96.52" y="111.76" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="C3" gate="G$1" pin="+"/>
<wire x1="154.94" y1="101.6" x2="149.86" y2="101.6" width="0.1524" layer="91"/>
<label x="149.86" y="101.6" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="C2" gate="G$1" pin="+"/>
<wire x1="154.94" y1="114.3" x2="149.86" y2="114.3" width="0.1524" layer="91"/>
<label x="149.86" y="114.3" size="1.778" layer="95" rot="R180"/>
</segment>
</net>
<net name="EN" class="0">
<segment>
<pinref part="J5" gate="G$1" pin="3"/>
<wire x1="25.4" y1="91.44" x2="20.32" y2="91.44" width="0.1524" layer="91"/>
<label x="20.32" y="91.44" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="J7" gate="G$1" pin="1"/>
<wire x1="101.6" y1="160.02" x2="96.52" y2="160.02" width="0.1524" layer="91"/>
<label x="96.52" y="160.02" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="J8" gate="G$1" pin="3"/>
<wire x1="101.6" y1="132.08" x2="96.52" y2="132.08" width="0.1524" layer="91"/>
<label x="96.52" y="132.08" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="R5" gate="G$1" pin="1"/>
<wire x1="154.94" y1="134.62" x2="149.86" y2="134.62" width="0.1524" layer="91"/>
<label x="149.86" y="134.62" size="1.778" layer="95" rot="R180"/>
</segment>
</net>
<net name="GND" class="0">
<segment>
<pinref part="J1" gate="G$1" pin="2"/>
<wire x1="25.4" y1="157.48" x2="20.32" y2="157.48" width="0.1524" layer="91"/>
<label x="20.32" y="157.48" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="J2" gate="G$1" pin="2"/>
<wire x1="25.4" y1="142.24" x2="20.32" y2="142.24" width="0.1524" layer="91"/>
<label x="20.32" y="142.24" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="J3" gate="G$1" pin="2"/>
<wire x1="25.4" y1="127" x2="20.32" y2="127" width="0.1524" layer="91"/>
<label x="20.32" y="127" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="J4" gate="G$1" pin="2"/>
<wire x1="25.4" y1="111.76" x2="20.32" y2="111.76" width="0.1524" layer="91"/>
<label x="20.32" y="111.76" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="J8" gate="G$1" pin="1"/>
<wire x1="101.6" y1="137.16" x2="96.52" y2="137.16" width="0.1524" layer="91"/>
<label x="96.52" y="137.16" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="J9" gate="G$1" pin="1"/>
<wire x1="101.6" y1="114.3" x2="96.52" y2="114.3" width="0.1524" layer="91"/>
<label x="96.52" y="114.3" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="R2" gate="G$1" pin="2"/>
<wire x1="106.68" y1="76.2" x2="111.76" y2="76.2" width="0.1524" layer="91"/>
<label x="111.76" y="76.2" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="C1" gate="G$1" pin="2"/>
<wire x1="106.68" y1="63.5" x2="111.76" y2="63.5" width="0.1524" layer="91"/>
<label x="111.76" y="63.5" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="R3" gate="G$1" pin="2"/>
<wire x1="165.1" y1="160.02" x2="170.18" y2="160.02" width="0.1524" layer="91"/>
<label x="170.18" y="160.02" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="R4" gate="G$1" pin="2"/>
<wire x1="165.1" y1="147.32" x2="170.18" y2="147.32" width="0.1524" layer="91"/>
<label x="170.18" y="147.32" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="R5" gate="G$1" pin="2"/>
<wire x1="165.1" y1="134.62" x2="170.18" y2="134.62" width="0.1524" layer="91"/>
<label x="170.18" y="134.62" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="C3" gate="G$1" pin="-"/>
<wire x1="165.1" y1="101.6" x2="170.18" y2="101.6" width="0.1524" layer="91"/>
<label x="170.18" y="101.6" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="C2" gate="G$1" pin="-"/>
<wire x1="165.1" y1="114.3" x2="170.18" y2="114.3" width="0.1524" layer="91"/>
<label x="170.18" y="114.3" size="1.778" layer="95"/>
</segment>
</net>
<net name="LPWM" class="0">
<segment>
<pinref part="J5" gate="G$1" pin="2"/>
<wire x1="25.4" y1="93.98" x2="20.32" y2="93.98" width="0.1524" layer="91"/>
<label x="20.32" y="93.98" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="J7" gate="G$1" pin="2"/>
<wire x1="101.6" y1="157.48" x2="96.52" y2="157.48" width="0.1524" layer="91"/>
<label x="96.52" y="157.48" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="R4" gate="G$1" pin="1"/>
<wire x1="154.94" y1="147.32" x2="149.86" y2="147.32" width="0.1524" layer="91"/>
<label x="149.86" y="147.32" size="1.778" layer="95" rot="R180"/>
</segment>
</net>
<net name="RPWM" class="0">
<segment>
<pinref part="J5" gate="G$1" pin="1"/>
<wire x1="25.4" y1="96.52" x2="20.32" y2="96.52" width="0.1524" layer="91"/>
<label x="20.32" y="96.52" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="J7" gate="G$1" pin="3"/>
<wire x1="101.6" y1="154.94" x2="96.52" y2="154.94" width="0.1524" layer="91"/>
<label x="96.52" y="154.94" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="R3" gate="G$1" pin="1"/>
<wire x1="154.94" y1="160.02" x2="149.86" y2="160.02" width="0.1524" layer="91"/>
<label x="149.86" y="160.02" size="1.778" layer="95" rot="R180"/>
</segment>
</net>
<net name="SERVO" class="0">
<segment>
<pinref part="J6" gate="G$1" pin="1"/>
<wire x1="25.4" y1="76.2" x2="20.32" y2="76.2" width="0.1524" layer="91"/>
<label x="20.32" y="76.2" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="J9" gate="G$1" pin="3"/>
<wire x1="101.6" y1="109.22" x2="96.52" y2="109.22" width="0.1524" layer="91"/>
<label x="96.52" y="109.22" size="1.778" layer="95" rot="R180"/>
</segment>
</net>
<net name="VBAT" class="0">
<segment>
<pinref part="J1" gate="G$1" pin="1"/>
<wire x1="25.4" y1="160.02" x2="20.32" y2="160.02" width="0.1524" layer="91"/>
<label x="20.32" y="160.02" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="J2" gate="G$1" pin="1"/>
<wire x1="25.4" y1="144.78" x2="20.32" y2="144.78" width="0.1524" layer="91"/>
<label x="20.32" y="144.78" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="R1" gate="G$1" pin="1"/>
<wire x1="96.52" y1="88.9" x2="91.44" y2="88.9" width="0.1524" layer="91"/>
<label x="91.44" y="88.9" size="1.778" layer="95" rot="R180"/>
</segment>
</net>
<net name="VSENSE" class="0">
<segment>
<pinref part="J6" gate="G$1" pin="2"/>
<wire x1="25.4" y1="73.66" x2="20.32" y2="73.66" width="0.1524" layer="91"/>
<label x="20.32" y="73.66" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="R1" gate="G$1" pin="2"/>
<wire x1="106.68" y1="88.9" x2="111.76" y2="88.9" width="0.1524" layer="91"/>
<label x="111.76" y="88.9" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="R2" gate="G$1" pin="1"/>
<wire x1="96.52" y1="76.2" x2="91.44" y2="76.2" width="0.1524" layer="91"/>
<label x="91.44" y="76.2" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="C1" gate="G$1" pin="1"/>
<wire x1="96.52" y1="63.5" x2="91.44" y2="63.5" width="0.1524" layer="91"/>
<label x="91.44" y="63.5" size="1.778" layer="95" rot="R180"/>
</segment>
</net>
</nets>
</sheet>
</sheets>
</schematic>
</drawing>
</eagle>
