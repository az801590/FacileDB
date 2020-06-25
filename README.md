# FacileDB
A simple and lightweight database implementation.

## Install
1. Make sure gcc and make has been installed.
2. Clone source code to any directory you want.
3. Execute "install.sh"

## StartUp
Execute "Facile" in the folder.

## Storage Entities
### Database
### Set
### Data
### Record
A pair of key and value.

### Commands
#### Normal commands
Command | description |
:-------|:------------|
list    | Show all exsited databases. |
show    | Show all exsited sets in current database. |
exit    | Exit FacileDb.

#### Select Databases
```
use dbName
```

#### PUT
```
setName.put "key1":"value1","key2":"value2"
```

#### FIND
```
//Everything in the set.
setName.find *=*

//key is equal to value.
setName.find "key"="value"

//key is not equal to value.
setName.find "key"!"value"

//All values belongs to certain key.
setName.find "key"=*

//All keys belongs to certain value.
setName.find *="value"

```

#### DELETE
```
setName.delete "key":"value"
```

#### INDEX
Index field would speed up executations than non-index field.
Index can be make before or after the data insert.
```
setName.makeIndex "key"
```