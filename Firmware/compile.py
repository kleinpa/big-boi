import os
import hashlib
import mimetypes

root = os.path.join(os.path.dirname(os.path.realpath(__file__)), "web")

files = []

for (directory, _, filenames) in os.walk(root):
  for f in filenames:
    files.append(os.path.relpath(os.path.join(directory, f), root))

def toCString(s, encoding='ascii'):
  # http://stackoverflow.com/a/14945195 by sth
#   if isinstance(s, unicode):
#      s = s.encode(encoding)
   result = ''
   for c in s:
      if not (32 <= ord(c) < 127) or c in ('\\', '"'):
         result += '\\%03o' % ord(c)
      else:
         result += c
   return result

file_spec = []

for path in files:
  http_path = "/" + path.replace("\\","/")
  mimetype, _ = mimetypes.guess_type(path)
  mimetype = mimetype or "application/octet-stream"
  signature = "http_" + hashlib.sha224(path.encode('utf-8')).hexdigest()
  if mimetype != "application/octet-stream":
    file_spec.append((http_path, mimetype, path, signature))

with open('http_content.h', 'w') as o:
  for http_path, mimetype, path, signature in file_spec:
    o.write("const String " + signature + " = ")
    with open(os.path.join(root, path), 'r') as i:
      empty = True
      for l in i:
        empty = False
        o.write('\n  \"')
        o.write(toCString(l))
        o.write('\"')
      if empty:
        o.write('\"\"')
    o.write(";\n")
  o.write("void load_http_content(ESP8266WebServer &server) {\n")
  for http_path, mimetype, path, signature in file_spec:
    o.write("  server.on(\"" + toCString(http_path) + "\", [&server]() { server.send(200, \"" + toCString(mimetype) + "\", " + signature + ");});\n")
    if http_path.endswith("index.html"):
      o.write("  server.on(\"" + toCString(http_path[:-10]) + "\", [&server]() { server.send(200, \"" + toCString(mimetype) + "\", " + signature + ");});\n")
  o.write("}")

for http_path, mimetype, path, signature in file_spec:
  print(http_path)
print("Processed {0} files".format(len(file_spec)))
