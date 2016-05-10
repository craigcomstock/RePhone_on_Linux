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

p = audiostream.play

s = audiostream.stop

hello = function()
	is_stereo = 0;
	bit_per_sample = 8;
	sample_frequency = 3;
	codec = 10;
	audiostream.play('hello.wav',is_stereo,bit_per_sample,sample_frequency,codec);
end


heat = function()
	is_stereo = 1;
	bit_per_sample = 16;
	sample_frequency = 6; -- 22050, 44100=6?
	-- wav codec 8 doesn't seem to work, raw PCM 10 does
	codec = 10; -- 5=mp3

	audiostream.play('heat.wav',is_stereo,bit_per_sample,sample_frequency,codec)
end

m = function()
	audiostream.play('music.mp3',-1,-1,-1,5);
end

fre = free = function() print(os.mem()) end
