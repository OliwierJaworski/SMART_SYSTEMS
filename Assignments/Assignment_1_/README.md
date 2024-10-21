# Opdracht 1: RTSP Stream (Deepstream)

## Instructies voor de opdracht:

Bouw een C++ applicatie die het toelaat om een RTSP Stream te ontvangen en deze terug uit te zenden op een andere poort. De applicatie dient gebruik te maken van Deepstream en te draaien op de Jetson Nano.

### Teststreams in het labo op netwerk TheEdge:

- **TheEdgeCam01**
  - RTSP HQ Stream: `rtsp://192.168.2.11:554/stream1`
  - RTSP LQ Stream: `rtsp://192.168.2.11:554/stream2`
  
- **TheEdgeCam02 (labobank Rechts)**
  - RTSP HQ Stream: `rtsp://192.168.2.12:554/stream1`
  - RTSP LQ Stream: `rtsp://192.168.2.12:554/stream2`
  
- **TheEdgeCam03 (labobank Links)**
  - RTSP HQ Stream: `rtsp://192.168.2.13:554/stream1`
  - RTSP LQ Stream: `rtsp://192.168.2.13:554/stream2`

### Beoordeling:
- [ ] RTSP Stream input werkt: **2 punten**
- [ ] RTSP Output stream werkt: **2 punten**
- [ ] Deepstream implementatie: **3 punten**

### Extra functionaliteiten:
- [ ] Multiple RTSP Stream input en output mogelijkheid bij opstart (via YAML file): **1 punt**
- [ ] Python versie van je project: **2 punten**

### Inleveren:
- Youtube link met demo van werkende versie
- Github link met C++ code en eventueel Python code
- Gebruikte versie Jetpack, Deepstream, Python en een requirements file voor Python