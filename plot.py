import matplotlib.pyplot as plt

time=[]
nbytes=[]

with open('videodownloadinput.txt') as fp:  
   line = fp.readline()
   ls=line.split(" ")
   time.append(ls[0])
   nbytes.append(ls[1])

plt.plot(time,nbytes,'bo')
plt.show()