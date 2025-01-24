#!/usr/bin/env python3

# Importar módulo CGI
import cgi

# Encabezado HTTP
  print("Content-Type: text/html\n")

# Contenido HTML
print("""
<html>
  <head>
    <title>Prueba CGI</title>
  </head>
  <body>
    <h1>¡Prueba exitosa de CGI!</h1>
    <p>Este script se está ejecutando en el servidor correctamente.</p>
  </body>
</html>
""")