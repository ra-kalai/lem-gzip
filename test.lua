local lgz = require 'lem.gzip.core'
local utils = require 'lem.utils'

local test = 'a'
local s
local decompress, err = lgz.decompress('coin',2)

if err ~= 'decompress fail' then
  print('woot decompress should not work')
	os.exit(1)
end

s = test:rep(100)
for i=1, 1000 do
	local compressed = lgz.compress(s)
	local decompress, err = lgz.decompress(compressed)
end

for i=1, 100000 do
	print('1', i)
	s = test:rep(i)
	print('2', i)
	local compressed = lgz.compress(s)
	local decompress , err = lgz.decompress(table.concat({('z'):rep(i), compressed}), i+1)

	if s ~= decompress or err then
		print("err decompress fail at", i)
		os.exit(1)
	end
end



--io.write(b)
