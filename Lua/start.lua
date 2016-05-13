h = function (x)
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

clear = function ()
	screen.set_color(0)
	screen.fill(0,0,240,240)
	screen.update()
end

show = function (msg)
	screen.set_color(255)
	screen.text(0,120,msg)
	screen.update()
end

peek = os.peek
poke = os.poke
shutdown = os.shutdown
reboot = os.reboot

vm_malloc = os.vm_malloc
vm_free = os.vm_free
vm_malloc_dma = os.vm_malloc_dma

malloc = os.malloc
free = os.free

init = screen.init
mem = os.mem

