# myDB

## Select databases
List all db files
```
list
```

Select or create a db file
```
use <dbFileName>
```

## INSERT
### Insert with file
file format
```
@url:http://www.gogle.com
@count:123
@
@url:http://www.yahoo.com
@count:234
@
```

command
```
put <fileName>
```

## Index
### make index
Sequence index.
```
index <column>
```


## Find
### Find all data
```
find *
```
### Find with certain key-value
```
find key:value
```


## Delete
### Delete all
```
delete *
```
### Delete with certain key-value
```
delete key:value
```