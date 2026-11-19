"""
Exporta los archivos de fabricación de la PCB del auto (RX) a partir de la misma
geometría que generar_pcb.py (pads, pistas y plano de GND):

  fabricacion/cdio_rx_cobre_bottom.gbr   cobre (capa Bottom, vista desde ARRIBA)
  fabricacion/cdio_rx_contorno.gbr       contorno de la placa (90 x 60 mm)
  fabricacion/cdio_rx_taladros_pth.xln   taladros de componentes (Excellon)
  fabricacion/cdio_rx_taladros_npth.xln  agujeros de fijación M3 (Excellon)
  fabricacion/vista_cobre_bottom.png     imagen del cobre para revisar
  cdio_rx_fabricacion.zip                todo lo anterior + LEEME.txt

Requiere shapely y matplotlib.  Uso:  python exportar_gerber.py
"""
import os
import zipfile

from shapely.geometry import LineString, Point, box
from shapely.ops import unary_union
from shapely import affinity

import generar_pcb as g

AQUI = os.path.dirname(os.path.abspath(__file__))
SALIDA = os.path.join(AQUI, "fabricacion")

AISLAMIENTO = 0.8      # separación del plano de GND a todo lo que no es GND
ANCHO_RAYO = 0.8       # rayos térmicos de los pads de GND
MARGEN_BORDE = 0.6     # el plano empieza a 0,6 mm del borde
ANCHO_MIN_PLANO = 0.5  # se eliminan zonas del plano más delgadas que esto
RESOLUCION = 32


# ---------------------------------------------------------------------------
# Geometría de cobre
# ---------------------------------------------------------------------------
def pads_placa():
    """(nombre, red, x, y, forma, ancho, alto, drill)"""
    out = []
    for parte in g.PARTES:
        nombre, ds, _, x, y, rot, padred = parte
        for p in g.PAQUETES[g.DEVICESETS[ds][1]]["pads"]:
            px, py = g.pos_pad(parte, p["name"])
            d = p["diam"]
            if p["shape"] == "long":
                # alargado al doble a lo largo de su eje; pad R90 + elemento R90 -> eje x
                giro = (90 if p["rot"] == "R90" else 0) + {"R0": 0, "R90": 90, "R180": 180, "R270": 270}[rot]
                horizontal = (giro % 180) == 0
                w, h = (2 * d, d) if horizontal else (d, 2 * d)
                out.append((f"{nombre}.{p['name']}", padred[p["name"]], px, py, "O", w, h, p["drill"]))
            else:
                out.append((f"{nombre}.{p['name']}", padred[p["name"]], px, py, "C", d, d, p["drill"]))
    return out


def forma_pad(x, y, forma, w, h):
    if forma == "C":
        return Point(x, y).buffer(w / 2, RESOLUCION)
    r = min(w, h) / 2
    largo = max(w, h) / 2 - r
    eje = LineString([(x - largo, y), (x + largo, y)]) if w >= h else LineString([(x, y - largo), (x, y + largo)])
    return eje.buffer(r, RESOLUCION)


def pistas():
    out = []
    for red, ancho, pts in g.RUTAS:
        pts = [g.punto(p) for p in pts]
        for a, b in zip(pts, pts[1:]):
            out.append((red, ancho, a, b))
    return out


def plano_gnd(pads, segs):
    zona = box(MARGEN_BORDE, MARGEN_BORDE, g.ANCHO - MARGEN_BORDE, g.ALTO - MARGEN_BORDE)
    for x, y in g.AGUJEROS:
        zona = zona.difference(Point(x, y).buffer(g.DRILL_AGUJERO / 2 + AISLAMIENTO, RESOLUCION))

    ajeno = []
    for _, red, x, y, f, w, h, _ in pads:
        ajeno.append(forma_pad(x, y, f, w, h).buffer(AISLAMIENTO, RESOLUCION))
    for red, ancho, a, b in segs:
        ajeno.append(LineString([a, b]).buffer(ancho / 2 + AISLAMIENTO, RESOLUCION))
    plano = zona.difference(unary_union(ajeno))
    # elimina zonas más delgadas que el ancho mínimo (no se pueden fresar)
    plano = plano.buffer(-ANCHO_MIN_PLANO / 2, RESOLUCION).buffer(ANCHO_MIN_PLANO / 2, RESOLUCION)

    # rayos térmicos: unen cada pad de GND con el plano
    bloqueo = unary_union([forma_pad(x, y, f, w, h).buffer(AISLAMIENTO, RESOLUCION)
                           for _, red, x, y, f, w, h, _ in pads if red != "GND"] +
                          [LineString([a, b]).buffer(an / 2 + AISLAMIENTO, RESOLUCION)
                           for red, an, a, b in segs if red != "GND"])
    rayos = []
    for _, red, x, y, f, w, h, _ in pads:
        if red != "GND":
            continue
        L = max(w, h) / 2 + AISLAMIENTO + 0.6
        for dx, dy in [(1, 0), (-1, 0), (0, 1), (0, -1)]:
            r = LineString([(x, y), (x + dx * L, y + dy * L)]).buffer(ANCHO_RAYO / 2, 4, cap_style="flat")
            rayos.append(r.difference(bloqueo).intersection(zona))
    gnd_pads = [forma_pad(x, y, f, w, h) for _, red, x, y, f, w, h, _ in pads if red == "GND"]
    total = unary_union([plano] + rayos + gnd_pads)

    # solo se conservan las islas que tocan algún pad de GND (sin huérfanos)
    islas = list(total.geoms) if total.geom_type == "MultiPolygon" else [total]
    centros = [Point(x, y) for _, red, x, y, *_ in pads if red == "GND"]
    utiles = [i for i in islas if any(i.contains(c) for c in centros)]
    return utiles, centros


# ---------------------------------------------------------------------------
# Escritura Gerber RS-274X / Excellon
# ---------------------------------------------------------------------------
def c(v):
    return str(int(round(v * 1e6)))


class Gerber:
    def __init__(self, nombre_capa, funcion):
        self.lineas = ["G04 CDIO - PCB del auto (RX), generado por exportar_gerber.py*",
                       f"%TF.FileFunction,{funcion}*%", "%TF.FilePolarity,Positive*%",
                       "%FSLAX46Y46*%", "%MOMM*%", f"G04 {nombre_capa}*", "%LPD*%"]
        self.aperturas = {}
        self.cuerpo = []

    def apertura(self, clave):
        if clave not in self.aperturas:
            n = 10 + len(self.aperturas)
            self.aperturas[clave] = n
        return self.aperturas[clave]

    def usar(self, clave):
        self.cuerpo.append(f"D{self.apertura(clave)}*")

    def flash(self, x, y):
        self.cuerpo.append(f"X{c(x)}Y{c(y)}D03*")

    def linea(self, a, b):
        self.cuerpo += ["G01*", f"X{c(a[0])}Y{c(a[1])}D02*", f"X{c(b[0])}Y{c(b[1])}D01*"]

    def region(self, anillo, polaridad="D"):
        pts = list(anillo.coords)
        self.cuerpo += [f"%LP{polaridad}*%", "G36*", f"X{c(pts[0][0])}Y{c(pts[0][1])}D02*", "G01*"]
        self.cuerpo += [f"X{c(x)}Y{c(y)}D01*" for x, y in pts[1:]]
        self.cuerpo += ["G37*", "%LPD*%"]

    def texto(self):
        defs = []
        for (tipo, *dims), n in self.aperturas.items():
            if tipo == "C":
                defs.append(f"%ADD{n}C,{dims[0]:.4f}*%")
            else:
                defs.append(f"%ADD{n}O,{dims[0]:.4f}X{dims[1]:.4f}*%")
        return "\n".join(self.lineas + defs + self.cuerpo + ["M02*"]) + "\n"


def exportar():
    os.makedirs(SALIDA, exist_ok=True)
    pads = pads_placa()
    segs = pistas()
    islas, centros_gnd = plano_gnd(pads, segs)

    # --- cobre Bottom
    gb = Gerber("Cobre Bottom (vista desde arriba)", "Copper,L2,Bot")
    for isla in sorted(islas, key=lambda p: -p.area):
        gb.region(isla.exterior, "D")
        for hueco in isla.interiors:
            gb.region(hueco, "C")
    for red, ancho, a, b in segs:
        gb.usar(("C", ancho))
        gb.linea(a, b)
    for _, red, x, y, f, w, h, _ in pads:
        gb.usar(("C", w) if f == "C" else ("O", w, h))
        gb.flash(x, y)
    with open(os.path.join(SALIDA, "cdio_rx_cobre_bottom.gbr"), "w") as fh:
        fh.write(gb.texto())

    # --- contorno
    gp = Gerber("Contorno de la placa", "Profile,NP")
    gp.usar(("C", 0.1))
    esq = [(0, 0), (g.ANCHO, 0), (g.ANCHO, g.ALTO), (0, g.ALTO), (0, 0)]
    for a, b in zip(esq, esq[1:]):
        gp.linea(a, b)
    with open(os.path.join(SALIDA, "cdio_rx_contorno.gbr"), "w") as fh:
        fh.write(gp.texto())

    # --- taladros
    def excellon(nombre, agujeros, plated):
        herramientas = sorted({d for _, _, d in agujeros})
        lin = ["M48", "; CDIO - PCB del auto (RX)", "METRIC,TZ",
               f"; {'Con metalizado (componentes)' if plated else 'Sin metalizado (fijacion M3)'}"]
        lin += [f"T{i + 1}C{d:.3f}" for i, d in enumerate(herramientas)]
        lin += ["%", "G90", "G05"]
        for i, d in enumerate(herramientas):
            lin.append(f"T{i + 1}")
            lin += [f"X{x:.3f}Y{y:.3f}" for x, y, dd in agujeros if dd == d]
        lin.append("M30")
        with open(os.path.join(SALIDA, nombre), "w") as fh:
            fh.write("\n".join(lin) + "\n")

    excellon("cdio_rx_taladros_pth.xln", [(x, y, dr) for _, _, x, y, _, _, _, dr in pads], True)
    excellon("cdio_rx_taladros_npth.xln", [(x, y, g.DRILL_AGUJERO) for x, y in g.AGUJEROS], False)

    return pads, segs, islas, centros_gnd


def verificar_y_dibujar(pads, segs, islas, centros_gnd):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.patches import Polygon as MplPoly

    plano = unary_union(islas)
    sin_conectar = [n for n, red, x, y, *_ in pads if red == "GND" and not plano.contains(Point(x, y))]
    componentes = len(islas)

    fig, ax = plt.subplots(figsize=(11, 7.4))
    ax.add_patch(MplPoly([(0, 0), (g.ANCHO, 0), (g.ANCHO, g.ALTO), (0, g.ALTO)], fc="#f4ede0", ec="k"))
    cobre = unary_union([plano] + [LineString([a, b]).buffer(w / 2, RESOLUCION) for _, w, a, b in segs] +
                        [forma_pad(x, y, f, w, h) for _, _, x, y, f, w, h, _ in pads])
    polys = cobre.geoms if cobre.geom_type == "MultiPolygon" else [cobre]
    # el plano (más grande) primero: sus huecos no deben tapar las pistas que van dentro
    for poly in sorted(polys, key=lambda p: -p.area):
        ax.add_patch(MplPoly(list(poly.exterior.coords), fc="#c87533", ec="none"))
        for hueco in poly.interiors:
            ax.add_patch(MplPoly(list(hueco.coords), fc="#f4ede0", ec="none"))
    for _, _, x, y, _, _, _, dr in pads:
        ax.add_patch(plt.Circle((x, y), dr / 2, fc="w", ec="none"))
    for x, y in g.AGUJEROS:
        ax.add_patch(plt.Circle((x, y), g.DRILL_AGUJERO / 2, fc="w", ec="k"))
    ax.set_xlim(-2, g.ANCHO + 2); ax.set_ylim(-2, g.ALTO + 2); ax.set_aspect("equal")
    ax.set_title("Cobre Bottom (vista desde ARRIBA) - CDIO RX, 90 x 60 mm")
    ax.set_xlabel("mm")
    plt.tight_layout()
    plt.savefig(os.path.join(SALIDA, "vista_cobre_bottom.png"), dpi=140)
    return componentes, sin_conectar


LEEME = """PCB DEL AUTO (RX) - PROYECTO CDIO
=================================

Placa de una capa para fresar en CNC.

Tamano ............ 90 x 60 mm
Capa de cobre ..... SOLO Bottom (los componentes van por el lado sin cobre)
Pistas ............ 1,0 mm (senales) y 2,0 mm (potencia)
Separacion min. ... 0,6 mm entre cobres distintos
Plano de GND ...... rellena el resto del cobre, a 0,8 mm de las pistas

Archivos
--------
cdio_rx_cobre_bottom.gbr    Cobre Bottom. Esta dibujado visto desde ARRIBA:
                            para fresar por el lado de abajo hay que ESPEJARLO
                            (mirror en X) en el software del CNC.
cdio_rx_contorno.gbr        Contorno de corte de la placa.
cdio_rx_taladros_pth.xln    Taladros de componentes: 1,0 mm y 1,3 mm.
cdio_rx_taladros_npth.xln   4 agujeros de fijacion de 3,2 mm (tornillos M3).
vista_cobre_bottom.png      Imagen de referencia del cobre.

Unidades: milimetros. Origen (0,0) en la esquina inferior izquierda.
"""


if __name__ == "__main__":
    pads, segs, islas, centros = exportar()
    comp, sin_conectar = verificar_y_dibujar(pads, segs, islas, centros)
    with open(os.path.join(SALIDA, "LEEME.txt"), "w", encoding="utf-8") as fh:
        fh.write(LEEME)
    zip_path = os.path.join(AQUI, "cdio_rx_fabricacion.zip")
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as z:
        for f in sorted(os.listdir(SALIDA)):
            z.write(os.path.join(SALIDA, f), f)
    print(f"Islas del plano de GND: {comp} (debe ser 1)")
    print(f"Pads de GND sin conectar al plano: {sin_conectar or 'ninguno'}")
    print(f"Listo: {zip_path}")
