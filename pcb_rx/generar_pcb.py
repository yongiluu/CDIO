"""
Genera la PCB del auto (RX) del proyecto CDIO como archivos EAGLE 9 (.sch y .brd),
que Fusion Electronics abre directamente (Archivo > Abrir > Abrir desde mi equipo).

La placa es un "hub" de conexiones: todo entra y sale por borneras de tornillo
(KF301, paso 5,08 mm). Los módulos (ESP32 con su placa de borneras, LM2596 y
BTS7960) quedan fuera de la PCB y se conectan con cables. En la PCB solo van
borneras, el conector del servo y los componentes pasivos.

Pensada para fresar en CNC: UNA capa (Bottom), pistas de 1,0 mm (señales) y
2,0 mm (potencia), separación mínima 0,6 mm y un plano de GND que rellena el resto.

Uso:  python generar_pcb.py      -> escribe cdio_rx.sch y cdio_rx.brd en esta carpeta
"""
import math
import os
from xml.sax.saxutils import escape

AQUI = os.path.dirname(os.path.abspath(__file__))

# ---------------------------------------------------------------------------
# Placa
# ---------------------------------------------------------------------------
ANCHO, ALTO = 90.0, 60.0                     # mm
AGUJEROS = [(4, 4), (86, 4), (4, 56), (86, 56)]  # M3, Ø3,2
DRILL_AGUJERO = 3.2

W_SENAL = 1.0
W_POTENCIA = 2.0
W_5V_HEADER = 1.6   # tramo del 5 V que pasa entre los pines del conector del servo

# ---------------------------------------------------------------------------
# Paquetes (footprints). Pads relativos al pad 1; cuerpo con el lado de entrada
# del cable de las borneras hacia -y.
# ---------------------------------------------------------------------------
def pads_linea(n, paso, diam, drill, shape="round", rot="R0"):
    return [dict(name=str(i + 1), x=i * paso, y=0.0, diam=diam, drill=drill, shape=shape, rot=rot)
            for i in range(n)]

PAQUETES = {
    "KF301-2": dict(
        pads=pads_linea(2, 5.08, 2.8, 1.3),
        silk=[("rect", -2.54, -4.1, 7.62, 3.5), ("line", -2.54, -2.8, 7.62, -2.8)],
        desc="Bornera de tornillo 2 polos, paso 5,08 mm (KF301-2P)"),
    "KF301-3": dict(
        pads=pads_linea(3, 5.08, 2.8, 1.3),
        silk=[("rect", -2.54, -4.1, 12.7, 3.5), ("line", -2.54, -2.8, 12.7, -2.8)],
        desc="Bornera de tornillo 3 polos, paso 5,08 mm (KF301-3P)"),
    "HDR1X3": dict(
        pads=pads_linea(3, 2.54, 1.9, 1.0, shape="long", rot="R90"),
        silk=[("rect", -1.27, -2.2, 6.35, 2.2)],
        desc="Tira de 3 pines macho 2,54 mm (conector del servo)"),
    "R-0207/5V": dict(
        pads=pads_linea(2, 5.08, 2.4, 1.0),
        silk=[("circle", 0, 0, 1.6)],
        desc="Resistencia 1/4 W de pie, paso 5,08 mm"),
    "R-0207/10": dict(
        pads=pads_linea(2, 10.16, 2.4, 1.0),
        silk=[("rect", 2.2, -1.2, 7.96, 1.2)],
        desc="Resistencia 1/4 W acostada, paso 10,16 mm"),
    "R-0207/15": dict(
        pads=pads_linea(2, 15.24, 2.4, 1.0),
        silk=[("rect", 4.5, -1.2, 10.74, 1.2)],
        desc="Resistencia 1/4 W acostada, paso 15,24 mm (pasa por encima de pistas)"),
    "C-5": dict(
        pads=pads_linea(2, 5.08, 2.4, 1.0),
        silk=[("rect", 1.0, -1.5, 4.08, 1.5)],
        desc="Condensador cerámico, paso 5,08 mm"),
    "CPOL-5-D8": dict(
        pads=pads_linea(2, 5.08, 2.4, 1.0),
        silk=[("circle", 2.54, 0, 4.0), ("text+", -1.6, 1.4)],
        desc="Electrolítico Ø8 mm, paso 5,08 mm (pad 1 = +)"),
    "CPOL-5-D10": dict(
        pads=pads_linea(2, 5.08, 2.4, 1.0),
        silk=[("circle", 2.54, 0, 5.0), ("text+", -1.6, 1.4)],
        desc="Electrolítico Ø10 mm, paso 5,08 mm (pad 1 = +)"),
}

# deviceset -> (símbolo, paquete, prefijo, usa valor)
DEVICESETS = {
    "TB2": ("CON2", "KF301-2", "J", False),
    "TB3": ("CON3", "KF301-3", "J", False),
    "SERVO": ("CON3", "HDR1X3", "J", False),
    "R5": ("RES", "R-0207/5V", "R", True),
    "R10": ("RES", "R-0207/10", "R", True),
    "R15": ("RES", "R-0207/15", "R", True),
    "C5": ("CAP", "C-5", "C", True),
    "CP8": ("CPOL", "CPOL-5-D8", "C", True),
    "CP10": ("CPOL", "CPOL-5-D10", "C", True),
}

# ---------------------------------------------------------------------------
# Partes: nombre, deviceset, valor, x, y (pad 1), rotación, pads -> red
# ---------------------------------------------------------------------------
PARTES = [
    # Fila inferior: potencia (cable sale hacia abajo)
    ("J1", "TB2", "BATERIA", 24, 8, "R0", {"1": "VBAT", "2": "GND"}),
    ("J2", "TB2", "LM2596_IN", 38, 8, "R0", {"1": "VBAT", "2": "GND"}),
    ("J3", "TB2", "LM2596_OUT", 54, 8, "R0", {"1": "5V", "2": "GND"}),
    # Columna izquierda: hacia la placa de borneras del ESP32 (cable sale a la izquierda)
    ("J5", "TB3", "ESP32_MOTOR", 9, 50, "R270", {"1": "RPWM", "2": "LPWM", "3": "EN"}),
    ("J6", "TB2", "ESP32_SERVO_BAT", 9, 31, "R270", {"1": "SERVO", "2": "VSENSE"}),
    ("J4", "TB2", "ESP32_POWER", 9, 17, "R270", {"1": "5V", "2": "GND"}),
    # Columna derecha: hacia el BTS7960 (cable sale a la derecha)
    ("J7", "TB3", "BTS7960_A", 81, 39.84, "R90", {"1": "EN", "2": "LPWM", "3": "RPWM"}),
    ("J8", "TB3", "BTS7960_B", 81, 20.84, "R90", {"1": "GND", "2": "5V", "3": "EN"}),
    # Servo (enchufe directo: café GND, rojo 5V, naranjo señal)
    ("J9", "SERVO", "SERVO", 82, 10, "R90", {"1": "GND", "2": "5V", "3": "SERVO"}),
    # Divisor de batería: VBAT -> 56k -> VSENSE -> 10k -> GND
    ("R1", "R15", "56k", 32, 13, "R90", {"1": "VBAT", "2": "VSENSE"}),
    ("R2", "R5", "10k", 26, 28.24, "R270", {"1": "VSENSE", "2": "GND"}),
    ("C1", "C5", "100nF", 20, 28.24, "R270", {"1": "VSENSE", "2": "GND"}),
    # Pull-down 10k del BTS7960 (el cuerpo pasa por encima de las otras pistas)
    ("R3", "R15", "10k", 20, 50, "R270", {"1": "RPWM", "2": "GND"}),
    ("R4", "R10", "10k", 26, 44.92, "R270", {"1": "LPWM", "2": "GND"}),
    ("R5", "R5", "10k", 32, 39.84, "R270", {"1": "EN", "2": "GND"}),
    # Condensadores del 5 V
    ("C3", "CP8", "100uF", 17, 17, "R270", {"1": "5V", "2": "GND"}),
    ("C2", "CP10", "1000uF", 67, 12.54, "R270", {"1": "5V", "2": "GND"}),
]


def rotar(x, y, rot):
    return {"R0": (x, y), "R90": (-y, x), "R180": (-x, -y), "R270": (y, -x)}[rot]


def pos_pad(parte, pad):
    nombre, ds, _, x, y, rot, _ = parte
    paquete = PAQUETES[DEVICESETS[ds][1]]
    p = next(p for p in paquete["pads"] if p["name"] == pad)
    dx, dy = rotar(p["x"], p["y"], rot)
    return (round(x + dx, 4), round(y + dy, 4))


P = {pt[0]: pt for pt in PARTES}


def pad(ref):
    nombre, num = ref.split(".")
    return pos_pad(P[nombre], num)


# ---------------------------------------------------------------------------
# Ruteo (capa Bottom). Cada tramo: (red, ancho, [puntos]); un punto puede ser
# "PARTE.PAD" o una coordenada (x, y).
# ---------------------------------------------------------------------------
RUTAS = [
    ("RPWM", W_SENAL, ["J5.1", "R3.1", "J7.3"]),
    ("LPWM", W_SENAL, ["J5.2", "R4.1", "J7.2"]),
    ("EN", W_SENAL, ["J5.3", "R5.1", (75, 39.84), "J7.1"]),
    ("EN", W_SENAL, [(75, 39.84), (75, 31), "J8.3"]),
    ("SERVO", W_SENAL, ["J6.1", (70, 31), (70, 15.08), "J9.3"]),
    ("VSENSE", W_SENAL, ["R1.2", "R2.1", "C1.1", (13, 28.24), "J6.2"]),
    ("VBAT", W_POTENCIA, ["J1.1", (24, 13), "R1.1", (38, 13), "J2.1"]),
    ("5V", W_POTENCIA, ["J4.1", "C3.1", (54, 17), (54, 12.54), "J3.1"]),
    ("5V", W_POTENCIA, [(54, 12.54), "C2.1", (75, 12.54)]),
    ("5V", W_5V_HEADER, [(75, 12.54), "J9.2", (87, 12.54)]),
    ("5V", W_POTENCIA, [(87, 12.54), (87, 25.92), "J8.2"]),
]

# Textos de serigrafía (x, y, texto, tamaño, rot)
ETIQUETAS = [
    (30, 56.2, "CDIO - AUTO (RX) v1", 2.0, "R0"),
    (12.6, 50.5, "G25", 1.2, "R0"), (12.6, 45.4, "G26", 1.2, "R0"),
    (12.6, 40.3, "G27", 1.2, "R0"), (12.6, 31.5, "G13 SERVO", 1.2, "R0"),
    (12.6, 23.6, "G34 BAT", 1.2, "R0"), (12.6, 18.6, "ESP 5V", 1.2, "R0"),
    (12.6, 12.4, "ESP GND", 1.2, "R0"),
    (66.5, 50.5, "RPWM", 1.2, "R0"), (66.5, 45.4, "LPWM", 1.2, "R0"),
    (66.5, 36.0, "R_EN", 1.2, "R0"), (76.2, 32.2, "L_EN", 1.2, "R0"),
    (76.2, 27.1, "VCC", 1.2, "R0"), (76.2, 22.0, "GND", 1.2, "R0"),
    (71.5, 17.0, "SERVO S+-", 1.2, "R0"),
    (21.5, 1.2, "BAT+  GND", 1.2, "R0"), (35.5, 1.2, "IN+  IN-", 1.2, "R0"),
    (51.5, 1.2, "OUT+ OUT-", 1.2, "R0"),
    (2.0, 33.0, "ESP32", 1.6, "R90"), (89.0, 28.0, "BTS7960", 1.6, "R90"),
]

# ---------------------------------------------------------------------------
# Capas EAGLE
# ---------------------------------------------------------------------------
CAPAS = [
    (1, "Top", 4), (16, "Bottom", 1), (17, "Pads", 2), (18, "Vias", 2), (19, "Unrouted", 6),
    (20, "Dimension", 24), (21, "tPlace", 7), (22, "bPlace", 7), (23, "tOrigins", 15),
    (24, "bOrigins", 15), (25, "tNames", 7), (26, "bNames", 7), (27, "tValues", 7),
    (28, "bValues", 7), (29, "tStop", 7), (30, "bStop", 7), (31, "tCream", 7),
    (32, "bCream", 7), (35, "tGlue", 7), (36, "bGlue", 7), (39, "tKeepout", 4),
    (40, "bKeepout", 1), (41, "tRestrict", 4), (42, "bRestrict", 1), (43, "vRestrict", 2),
    (44, "Drills", 7), (45, "Holes", 7), (46, "Milling", 3), (47, "Measures", 7),
    (48, "Document", 7), (49, "Reference", 7), (51, "tDocu", 7), (52, "bDocu", 7),
    (88, "SimResults", 9), (89, "SimProbes", 9), (90, "Modules", 5), (91, "Nets", 2),
    (92, "Busses", 1), (93, "Pins", 2), (94, "Symbols", 4), (95, "Names", 7),
    (96, "Values", 7), (97, "Info", 7), (98, "Guide", 6),
]


def f(v):
    return f"{v:.4f}".rstrip("0").rstrip(".")


def xml_capas():
    return "<layers>\n" + "".join(
        f'<layer number="{n}" name="{nom}" color="{c}" fill="1" visible="yes" active="yes"/>\n'
        for n, nom, c in CAPAS) + "</layers>\n"


def xml_paquetes():
    out = ["<packages>\n"]
    for nombre, pq in PAQUETES.items():
        out.append(f'<package name="{escape(nombre)}">\n<description>{escape(pq["desc"])}</description>\n')
        for p in pq["pads"]:
            rot = f' rot="{p["rot"]}"' if p["rot"] != "R0" else ""
            out.append(f'<pad name="{p["name"]}" x="{f(p["x"])}" y="{f(p["y"])}" drill="{p["drill"]}" '
                       f'diameter="{p["diam"]}" shape="{p["shape"]}"{rot}/>\n')
        for s in pq["silk"]:
            if s[0] == "rect":
                _, x1, y1, x2, y2 = s
                for a, b in [((x1, y1), (x2, y1)), ((x2, y1), (x2, y2)), ((x2, y2), (x1, y2)), ((x1, y2), (x1, y1))]:
                    out.append(f'<wire x1="{f(a[0])}" y1="{f(a[1])}" x2="{f(b[0])}" y2="{f(b[1])}" width="0.2" layer="21"/>\n')
            elif s[0] == "line":
                _, x1, y1, x2, y2 = s
                out.append(f'<wire x1="{f(x1)}" y1="{f(y1)}" x2="{f(x2)}" y2="{f(y2)}" width="0.2" layer="21"/>\n')
            elif s[0] == "circle":
                _, x, y, r = s
                out.append(f'<circle x="{f(x)}" y="{f(y)}" radius="{f(r)}" width="0.2" layer="21"/>\n')
            elif s[0] == "text+":
                _, x, y = s
                out.append(f'<text x="{f(x)}" y="{f(y)}" size="1.2" layer="21">+</text>\n')
        out.append('<text x="0" y="4.2" size="1.2" layer="25">&gt;NAME</text>\n')
        out.append('<text x="0" y="-5.6" size="1" layer="27">&gt;VALUE</text>\n')
        out.append("</package>\n")
    out.append("</packages>\n")
    return "".join(out)


# Símbolos: pines como (nombre, x, y, rot)
SIMBOLOS = {
    "CON2": [("1", -5.08, 0, "R0"), ("2", -5.08, -2.54, "R0")],
    "CON3": [("1", -5.08, 0, "R0"), ("2", -5.08, -2.54, "R0"), ("3", -5.08, -5.08, "R0")],
    "RES": [("1", -5.08, 0, "R0"), ("2", 5.08, 0, "R180")],
    "CAP": [("1", -5.08, 0, "R0"), ("2", 5.08, 0, "R180")],
    "CPOL": [("+", -5.08, 0, "R0"), ("-", 5.08, 0, "R180")],
}
PIN_A_PAD = {"CPOL": {"+": "1", "-": "2"}}


def xml_simbolos():
    out = ["<symbols>\n"]
    for nombre, pines in SIMBOLOS.items():
        out.append(f'<symbol name="{nombre}">\n')
        if nombre.startswith("CON"):
            fondo = -2.54 * (len(pines) - 1) - 2.54
            for a, b in [((-2.54, 2.54), (5.08, 2.54)), ((5.08, 2.54), (5.08, fondo)),
                         ((5.08, fondo), (-2.54, fondo)), ((-2.54, fondo), (-2.54, 2.54))]:
                out.append(f'<wire x1="{f(a[0])}" y1="{f(a[1])}" x2="{f(b[0])}" y2="{f(b[1])}" width="0.254" layer="94"/>\n')
            out.append('<text x="-2.54" y="3.302" size="1.778" layer="95">&gt;NAME</text>\n')
            out.append(f'<text x="-2.54" y="{f(fondo - 2.54)}" size="1.778" layer="96">&gt;VALUE</text>\n')
        else:
            for a, b in [((-2.54, 1.016), (2.54, 1.016)), ((2.54, 1.016), (2.54, -1.016)),
                         ((2.54, -1.016), (-2.54, -1.016)), ((-2.54, -1.016), (-2.54, 1.016))]:
                out.append(f'<wire x1="{f(a[0])}" y1="{f(a[1])}" x2="{f(b[0])}" y2="{f(b[1])}" width="0.254" layer="94"/>\n')
            out.append('<text x="-2.54" y="1.778" size="1.778" layer="95">&gt;NAME</text>\n')
            out.append('<text x="-2.54" y="-3.556" size="1.778" layer="96">&gt;VALUE</text>\n')
        for pn, x, y, rot in pines:
            out.append(f'<pin name="{escape(pn)}" x="{f(x)}" y="{f(y)}" visible="pad" length="short" direction="pas" rot="{rot}"/>\n')
        out.append("</symbol>\n")
    out.append("</symbols>\n")
    return "".join(out)


def xml_devicesets():
    out = ["<devicesets>\n"]
    for nombre, (sim, paq, pref, valor) in DEVICESETS.items():
        uv = ' uservalue="yes"' if valor else ""
        out.append(f'<deviceset name="{nombre}" prefix="{pref}"{uv}>\n<gates>\n'
                   f'<gate name="G$1" symbol="{sim}" x="0" y="0"/>\n</gates>\n<devices>\n'
                   f'<device name="" package="{escape(paq)}">\n<connects>\n')
        for pn, *_ in SIMBOLOS[sim]:
            padn = PIN_A_PAD.get(sim, {}).get(pn, pn)
            out.append(f'<connect gate="G$1" pin="{escape(pn)}" pad="{padn}"/>\n')
        out.append('</connects>\n<technologies>\n<technology name=""/>\n</technologies>\n</device>\n</devices>\n</deviceset>\n')
    out.append("</devicesets>\n")
    return "".join(out)


def cabecera():
    return ('<?xml version="1.0" encoding="utf-8"?>\n<!DOCTYPE eagle SYSTEM "eagle.dtd">\n'
            '<eagle version="9.6.2">\n<drawing>\n<settings>\n<setting alwaysvectorfont="no"/>\n'
            '<setting verticaltext="up"/>\n</settings>\n'
            '<grid distance="0.635" unitdist="mm" unit="mm" style="lines" multiple="1" display="no" '
            'altdistance="0.127" altunitdist="mm" altunit="mm"/>\n' + xml_capas())


# ---------------------------------------------------------------------------
# Esquemático
# ---------------------------------------------------------------------------
def generar_sch():
    # Ubicación de los símbolos en la hoja (mm, múltiplos de 2,54)
    ubic = {
        "J1": (30.48, 160.02), "J2": (30.48, 144.78), "J3": (30.48, 129.54), "J4": (30.48, 114.3),
        "J5": (30.48, 96.52), "J6": (30.48, 76.2), "J7": (106.68, 160.02), "J8": (106.68, 137.16),
        "J9": (106.68, 114.3), "R1": (101.6, 88.9), "R2": (101.6, 76.2), "C1": (101.6, 63.5),
        "R3": (160.02, 160.02), "R4": (160.02, 147.32), "R5": (160.02, 134.62),
        "C2": (160.02, 114.3), "C3": (160.02, 101.6),
    }
    redes = {}
    for nombre, ds, valor, *_r, padred in PARTES:
        sim = DEVICESETS[ds][0]
        x0, y0 = ubic[nombre]
        for pn, px, py, _rot in SIMBOLOS[sim]:
            padn = PIN_A_PAD.get(sim, {}).get(pn, pn)
            red = padred[padn]
            ax, ay = x0 + px, y0 + py
            hacia = -5.08 if px < 0 else 5.08
            redes.setdefault(red, []).append((nombre, pn, ax, ay, ax + hacia, ay))

    out = [cabecera(), '<schematic xreflabel="%F%N/%S.%C%R" xrefpart="/%S.%C%R">\n',
           '<description>CDIO - PCB del auto (RX). Generado por pcb_rx/generar_pcb.py</description>\n',
           '<libraries>\n<library name="cdio">\n<description>Biblioteca del proyecto CDIO</description>\n',
           xml_paquetes(), xml_simbolos(), xml_devicesets(), "</library>\n</libraries>\n",
           '<attributes>\n</attributes>\n<variantdefs>\n</variantdefs>\n',
           '<classes>\n<class number="0" name="default" width="0" drill="0">\n</class>\n</classes>\n<parts>\n']
    for nombre, ds, valor, *_ in PARTES:
        out.append(f'<part name="{nombre}" library="cdio" deviceset="{ds}" device="" value="{escape(valor)}"/>\n')
    out.append("</parts>\n<sheets>\n<sheet>\n<plain>\n")
    out.append('<text x="30.48" y="182.88" size="3.81" layer="97">CDIO - Auto (RX): hub de conexiones</text>\n')
    out.append("</plain>\n<instances>\n")
    for nombre, *_ in PARTES:
        x, y = ubic[nombre]
        out.append(f'<instance part="{nombre}" gate="G$1" x="{f(x)}" y="{f(y)}"/>\n')
    out.append("</instances>\n<busses>\n</busses>\n<nets>\n")
    for red, pines in sorted(redes.items()):
        out.append(f'<net name="{escape(red)}" class="0">\n')
        for parte, pn, ax, ay, bx, by in pines:
            rot = ' rot="R180"' if bx < ax else ""
            out.append(f'<segment>\n<pinref part="{parte}" gate="G$1" pin="{escape(pn)}"/>\n'
                       f'<wire x1="{f(ax)}" y1="{f(ay)}" x2="{f(bx)}" y2="{f(by)}" width="0.1524" layer="91"/>\n'
                       f'<label x="{f(bx)}" y="{f(by)}" size="1.778" layer="95"{rot}/>\n</segment>\n')
        out.append("</net>\n")
    out.append("</nets>\n</sheet>\n</sheets>\n</schematic>\n</drawing>\n</eagle>\n")
    return "".join(out)


# ---------------------------------------------------------------------------
# Placa
# ---------------------------------------------------------------------------
REGLAS = {
    "layerSetup": "(1*16)", "mtCopper": "0.035mm 0.035mm", "mtIsolate": "1.5mm",
    "mdWireWire": "0.6mm", "mdWirePad": "0.6mm", "mdWireVia": "0.6mm", "mdPadPad": "0.6mm",
    "mdPadVia": "0.6mm", "mdViaVia": "0.6mm", "mdSmdPad": "0.6mm", "mdSmdVia": "0.6mm",
    "mdSmdSmd": "0.6mm", "mdCopperDimension": "0.5mm", "mdDrill": "0.4mm",
    "msWidth": "0.8mm", "msDrill": "0.8mm", "rvPadTop": "0.25", "rvPadInner": "0.25",
    # Anillo mínimo 0,4 mm: con 0,5 mm Fusion agrandaba los pads del conector del
    # servo (paso 2,54) a 2,0 mm y quedaban a 0,54 mm entre sí (DRC pide 0,6).
    "rvPadBottom": "0.25", "rlMinPadTop": "0.4mm", "rlMaxPadTop": "20mm",
    "rlMinPadInner": "0.4mm", "rlMaxPadInner": "20mm", "rlMinPadBottom": "0.4mm",
    "rlMaxPadBottom": "20mm", "psElongationLong": "100", "psElongationOffset": "100",
}


def punto(p):
    return pad(p) if isinstance(p, str) else (float(p[0]), float(p[1]))


def generar_brd():
    out = [cabecera(), "<board>\n<plain>\n"]
    for a, b in [((0, 0), (ANCHO, 0)), ((ANCHO, 0), (ANCHO, ALTO)), ((ANCHO, ALTO), (0, ALTO)), ((0, ALTO), (0, 0))]:
        out.append(f'<wire x1="{f(a[0])}" y1="{f(a[1])}" x2="{f(b[0])}" y2="{f(b[1])}" width="0" layer="20"/>\n')
    for x, y in AGUJEROS:
        out.append(f'<hole x="{f(x)}" y="{f(y)}" drill="{DRILL_AGUJERO}"/>\n')
    for x, y, txt, tam, rot in ETIQUETAS:
        r = f' rot="{rot}"' if rot != "R0" else ""
        out.append(f'<text x="{f(x)}" y="{f(y)}" size="{tam}" layer="21" ratio="12"{r}>{escape(txt)}</text>\n')
    out.append("</plain>\n<libraries>\n<library name=\"cdio\">\n" + xml_paquetes() + "</library>\n</libraries>\n")
    out.append('<attributes>\n</attributes>\n<variantdefs>\n</variantdefs>\n'
               '<classes>\n<class number="0" name="default" width="0" drill="0">\n</class>\n</classes>\n'
               '<designrules name="cdio_cnc">\n')
    for k, v in REGLAS.items():
        out.append(f'<param name="{k}" value="{v}"/>\n')
    out.append("</designrules>\n<autorouter>\n</autorouter>\n<elements>\n")
    for nombre, ds, valor, x, y, rot, _ in PARTES:
        r = f' rot="{rot}"' if rot != "R0" else ""
        out.append(f'<element name="{nombre}" library="cdio" package="{escape(DEVICESETS[ds][1])}" '
                   f'value="{escape(valor)}" x="{f(x)}" y="{f(y)}"{r}/>\n')
    out.append("</elements>\n<signals>\n")
    redes = {}
    for nombre, *_r, padred in PARTES:
        for pn, red in padred.items():
            redes.setdefault(red, []).append((nombre, pn))
    for red in sorted(redes):
        out.append(f'<signal name="{escape(red)}">\n')
        for parte, pn in redes[red]:
            out.append(f'<contactref element="{parte}" pad="{pn}"/>\n')
        for rred, ancho, pts in RUTAS:
            if rred != red:
                continue
            pts = [punto(p) for p in pts]
            for a, b in zip(pts, pts[1:]):
                out.append(f'<wire x1="{f(a[0])}" y1="{f(a[1])}" x2="{f(b[0])}" y2="{f(b[1])}" '
                           f'width="{ancho}" layer="16"/>\n')
        if red == "GND":
            m = 0.6
            out.append('<polygon width="0.5" layer="16" isolate="0.8" orphans="no" thermals="yes" rank="1">\n')
            for x, y in [(m, m), (ANCHO - m, m), (ANCHO - m, ALTO - m), (m, ALTO - m)]:
                out.append(f'<vertex x="{f(x)}" y="{f(y)}"/>\n')
            out.append("</polygon>\n")
        out.append("</signal>\n")
    out.append("</signals>\n</board>\n</drawing>\n</eagle>\n")
    return "".join(out)


# ---------------------------------------------------------------------------
# Verificación geométrica básica (cobre en Bottom)
# ---------------------------------------------------------------------------
def dist_punto_segmento(p, a, b):
    ax, ay = a; bx, by = b; px, py = p
    dx, dy = bx - ax, by - ay
    L = dx * dx + dy * dy
    t = 0 if L == 0 else max(0, min(1, ((px - ax) * dx + (py - ay) * dy) / L))
    return math.hypot(px - (ax + t * dx), py - (ay + t * dy))


def dist_segmentos(a, b, c, d):
    def cruzan(p1, p2, p3, p4):
        def o(a, b, c):
            return (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])
        return (o(p1, p2, p3) * o(p1, p2, p4) < 0) and (o(p3, p4, p1) * o(p3, p4, p2) < 0)
    if cruzan(a, b, c, d):
        return 0.0
    return min(dist_punto_segmento(a, c, d), dist_punto_segmento(b, c, d),
               dist_punto_segmento(c, a, b), dist_punto_segmento(d, a, b))


def radio_pad(p):
    # pads "long" del header: se aproxima por el semieje corto (dirección de los vecinos)
    return p["diam"] / 2


def verificar(min_clear=0.6):
    pads = []
    for parte in PARTES:
        nombre, ds, _, x, y, rot, padred = parte
        for p in PAQUETES[DEVICESETS[ds][1]]["pads"]:
            pads.append((f"{nombre}.{p['name']}", padred[p["name"]], pos_pad(parte, p["name"]), radio_pad(p)))
    segs = []
    for red, ancho, pts in RUTAS:
        pts = [punto(p) for p in pts]
        for a, b in zip(pts, pts[1:]):
            segs.append((red, ancho, a, b))
    errores = []
    for i, (r1, w1, a, b) in enumerate(segs):
        for r2, w2, c, d in segs[i + 1:]:
            if r1 != r2:
                g = dist_segmentos(a, b, c, d) - w1 / 2 - w2 / 2
                if g < min_clear:
                    errores.append(f"pista {r1} {a}-{b} vs pista {r2} {c}-{d}: {g:.2f} mm")
        for nom, red, pp, rad in pads:
            if red != r1:
                g = dist_punto_segmento(pp, a, b) - w1 / 2 - rad
                if g < min_clear:
                    errores.append(f"pista {r1} {a}-{b} vs pad {nom} ({red}): {g:.2f} mm")
    for i, (n1, r1, p1, ra) in enumerate(pads):
        for n2, r2, p2, rb in pads[i + 1:]:
            if r1 != r2:
                g = math.hypot(p1[0] - p2[0], p1[1] - p2[1]) - ra - rb
                if g < min_clear:
                    errores.append(f"pad {n1} vs pad {n2}: {g:.2f} mm")
    # Cada extremo de pista que dice tocar un pad debe coincidir con él
    for red, _, pts in RUTAS:
        for p in pts:
            if isinstance(p, str):
                parte, num = p.split(".")
                if P[parte][6][num] != red:
                    errores.append(f"{p} no pertenece a la red {red}")
    return errores


if __name__ == "__main__":
    errores = verificar()
    for e in errores:
        print("ERROR:", e)
    with open(os.path.join(AQUI, "cdio_rx.sch"), "w", encoding="utf-8") as fh:
        fh.write(generar_sch())
    with open(os.path.join(AQUI, "cdio_rx.brd"), "w", encoding="utf-8") as fh:
        fh.write(generar_brd())
    print(f"Listo: cdio_rx.sch y cdio_rx.brd ({len(PARTES)} partes, {len(errores)} errores de separación)")
