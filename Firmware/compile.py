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

def toCCharArray(f):
  bs = f.read()
  return (len(bs), "{" + ', '.join('0x{:02x}'.format(x) for x in bs) + "}")

file_spec = []

for path in files:
  http_path = "/" + path.replace("\\","/")
  mimetype, _ = mimetypes.guess_type(path)
  mimetype = mimetype or "application/octet-stream"
  signature = "http_" + hashlib.sha224(path.encode('utf-8')).hexdigest()
  with open(os.path.join(root, path), 'rb') as i:
    l, b = toCCharArray(i)
  if mimetype != "application/octet-stream":
    file_spec.append((http_path, mimetype, path, signature, l, b))

with open('http_content.h', 'w') as o:
  for http_path, mimetype, path, signature, l, b in file_spec:
    o.write("const char " + signature + "[" + str(l) + "] = " + b + ";\n")
  o.write("void load_http_content(ESP8266WebServer &server) {\n")
  for http_path, mimetype, path, signature, l, b in file_spec:
    o.write("  server.on(\"" + toCString(http_path) + "\", [&server]() { server.send_P(200, \"" + toCString(mimetype) + "\", " + signature + "," + str(l) + ");});\n")
    if http_path.endswith("index.html"):
      o.write("  server.on(\"" + toCString(http_path[:-10]) + "\", [&server]() { server.send_P(200, \"" + toCString(mimetype) + "\", " + signature + "," + str(l) + ");});\n")
  o.write("}")

for http_path, mimetype, path, signature, l, b in file_spec:
  print(http_path)
print("Processed {0} files".format(len(file_spec)))
