
python3 ./scripts/execute.py --flatten-dir './flattened' --no-compile --no-test
tar -czvf ./flattened.tar.gz ./flattened
git checkout submit
rm -rf ./flattened
tar -xzvf ./flattened.tar.gz
rm ./flattened.tar.gz
git add ./flattened
git commit -m "submit"
