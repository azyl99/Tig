import json
import re
import sys

input = open("../tiger/"+sys.argv[1]+".txt", "r")
str = input.read()

pattern = re.compile(r'\[([^\[\]]*)')
str = re.sub(pattern, lambda match: "{'name':'"+match.group(1)+"','children':[", str)
pattern = re.compile(r']')
str = re.sub(pattern, lambda match: "]},", str)
pattern = re.compile(r",'children':\[\]")
str = re.sub(pattern, lambda match: "", str)

exec('a='+str)
Json = json.dumps(a[0])
output = open("../js-tree/"+sys.argv[1]+".json", "w")
output.write(Json)