import json
import re
import sys

input = open("../tiger/"+sys.argv[1]+".txt", "r")
str = input.read()
str = re.sub(r'\[([^\[\]]*)', lambda m: "{'name':'"+match.group(1)+"','children':[", str)
str = re.sub(r']', lambda m: "]},", str)
str = re.sub(r",'children':\[\]", lambda m: "", str)
exec('a='+str)
Json = json.dumps(a[0])
output = open("../js-tree/"+sys.argv[1]+".json", "w")
output.write(Json)