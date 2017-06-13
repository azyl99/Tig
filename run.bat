cd tiger
make
a test.tig
cd ../tools
@echo off
@echo start convert...
for %%i in (../tiger/output*.txt) do (
	python convert_tree_to_json.py %%~ni
)
@echo convert finished...
pause