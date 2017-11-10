import matplotlib.pyplot as plt

time=[]
nbytes=[]
fp=open('videodownloadinput.txt')
for line in fp:
   ls=line.split(" ")
   time.append(ls[0])
   nbytes.append(ls[1])

plt.plot(time,nbytes,'bo')
plt.show()
