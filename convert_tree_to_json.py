import json
import re

input = open("./tiger/output_syntax.txt", "r")
arrStr = input.read()

pattern = re.compile(r'\[([^\[\]]*)')
def replace(match):
	m = match.group(1)
	return "{'name':'"+m+"','children':["
arrStr = re.sub(pattern, replace, arrStr)
pattern = re.compile(r']')
def replace(match):
	return "]},"
arrStr = re.sub(pattern, replace, arrStr)
pattern = re.compile(r",'children':\[\]")
def replace(match):
	return ""
arrStr = re.sub(pattern, replace, arrStr)

exec('arr='+arrStr)
Json = json.dumps(arr[0])
output = open("./js-tree/output_syntax.json", "w")
output.write(Json)