import matplotlib.pyplot as plt

time=[]
nbytes=[]
time1=[]
nbytes1=[]
fp=open('input.txt')
for line in fp:  
   ls=line.split(" ")
   time.append(float(ls[0]))
   nbytes.append(float(ls[1])/1000)


fp1=open('output1.txt')
for line in fp1:  
   ls=line.split(" ")
   time1.append(float(ls[0])+5)
   nbytes1.append(float(ls[1])/1000)

fig = plt.figure()
fig.suptitle('No of KiloBytes vs Time(Input- Blue, Output- Green)', fontsize=14, fontweight='bold')

ax = fig.add_subplot(111)
fig.subplots_adjust(top=0.85)

ax.set_xlabel('Time')
ax.set_ylabel('Number of KiloBytes per second')
x_values = [100, 200, 300, 400, 500, 600, 700]
y_values = [50, 100,150,200,250,300,350,400,450,500,550]
plt.xticks(x_values)
plt.yticks(y_values)
plt.plot(time,nbytes,'bo',time1,nbytes1,'go')
plt.savefig('inputvsoutput1.png')
plt.show()
