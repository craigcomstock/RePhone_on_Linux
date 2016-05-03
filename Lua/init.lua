help = function (x)
	if x == nil then
		x = _G
	end
	for k,v in pairs(x) do
		print (k,v)
	end
end
ls = function (dir)
	os.list(dir)
end
