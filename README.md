# plist-parser
parsePlist : 解析plist内容(字符串),返回AnyValue*
parsePlistByFileName : 解析plist文件,返回AnyValue*
plist有多种数据节点类型，AnyValue是其基类，可转型为具体的派生类，然后进行相应数据操作