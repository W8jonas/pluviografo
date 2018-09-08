from visa import *
import time
import re

class dm(GpibInstrument):
    prim_addr = -1;
    sample_time=0.0006530386740331491712707182320442;
    op_statlst=['OPEN',    #just open, not initialize for test
		'CLOSE',   #shutdown,need reopen again
		'RUN',     #begin measure
		'RDY',     #stop running,data is ready
		'ERROR',   #instrument is in error state, need reset
		'LOSE'];   #can't contact with it
    op_stat='LOSE';
    def __init__(self,name='',timeout = 10):
        '''name: dm1 or dm2,please check tag on dm instruments
        '''
	try:
	    instr_lst0 = get_instruments_list();
	    instr_lst= [];
	    for i in range(0, len(instr_lst0)):
		if re.findall(r'GPIB', instr_lst0[i],re.M)!=[]:
		    instr_lst.append(instr_lst0[i]);
	    if len(instr_lst) == 0:
		print 'Find no instrument on GPIB Bus!';
		return;
	    self.prim_addr='';
	    for i in range(0, len(instr_lst)):
		instr = instrument(instr_lst[i]);
		answer = instr.ask('*IDN?');
		if answer.find('HEWLETT-PACKARD,34401A,0,10-5-2')!=-1:
                    if name in ['dm1','DM1','Dm1'] and instr_lst[i]=='GPIB0::6':
			self.prim_addr=instr_lst[i];
			print 'Find DM1, Address is %s'%instr_lst[i];
			break;
                    elif name in ['dm2','DM2','Dm2'] and instr_lst[i]=='GPIB0::22':
			self.prim_addr=instr_lst[i];
			print 'Find DM2, Address is %s'%instr_lst[i];
			break;
                    elif name =='':
                        print 'Find DM, Address is %s'%instr_lst[i];
                        self.prim_addr=instr_lst[i];
		        break;
	    if self.prim_addr == '':
                print 'Sorry, find no DM name:%s exist!'%name;
		return;
	    GpibInstrument.__init__(self,self.prim_addr);
	    print 'Open DM Successfully!';
	    self.op_stat = 'OPEN';
	    self.timeout = timeout;
	except:
	    print 'Fail to operate GPIB bus!'
	    return;
	print 'Initialize DM OK!'
    def get_result(self,name ,data_type = 'AVER'):
	if name == 'IDC':
	    return '%4.8f'%(float(self.ask("MEAS:CURR:DC? MAX")));
	elif name == 'VDC':
	    return '%4.8f'%(float(self.ask("MEAS:VOLT:DC? MAX")));
	elif name == 'IAC':
	    return '%4.8f'%(float(self.ask("MEAS:CURR:AC? MAX")));
	elif name == 'VAC':
	    return '%4.8f'%(float(self.ask("MEAS:VOLT:AC? MAX")));
	else:
	    return 'DATA NAME ERROR';

    def isbusy(self,timeout=10):
	for i in range(0,timeout):
	    if '1'==self.ask('*OPC?'):
	        return False;
    	    time.sleep(1);
	return True;

    def reset(self):
        self.write("*RST");
        self.write("*CLS");
	if False== self.isbusy():
	    print 'DM reset ok!'
	    return True;
        else:
            print 'DM reset timeout %d sec!'%timeout;
	    return False;


    def start(self,meastype='CURR',maxvalue=0.3,res=1E-3,sample_num=512):
        self.write("CONF:%s:DC DEF,DEF"%meastype);
        self.write("%s:DC:RANG %f"%(meastype,maxvalue));
        self.write("%s:DC:RES %f"%(meastype,res));
        self.write("%s:DC:NPLC MIN"%meastype);
        self.write("ZERO:AUTO OFF");
        self.write("DET:BAND MAX");
        self.write("TRIG:SOUR BUS");
        self.write("TRIG:DEL:AUTO OFF");
        self.write("TRIG:DEL 0");
        self.write("SAMP:COUN %d"%sample_num);
        self.write("TRIG:COUN 1");
        self.write("INIT");

    def getcurve(self):
        self.write("*TRG");
        result=self.ask("FETC?");
        result_lst=result.split(',');
        curve=[float(data) for data in result_lst];
        return curve;

    def start_rate(self,meastype='CURR',maxvalue=0.3,sample_num=512,sample_rate=0.001):
        self.reset();
        #self.write("DISP OFF");
        self.write("CONF:%s:DC DEF,DEF"%meastype);
        self.write("%s:DC:RANG %f"%(meastype,maxvalue));
        self.write("%s:DC:RES MAX"%meastype);
        self.write("%s:DC:NPLC MIN"%meastype);
        self.write("ZERO:AUTO OFF");
        self.write("%s:DC:RANG AUTO OFF"%meastype);
        self.write("CALC:STAT OFF");
        self.write("TRIG:SOUR IMM");
        delay=sample_rate-self.sample_time;
        self.write("TRIG:DEL %f"%delay); # sample delay = 1ms - sample time (0.6538461528ms)
        self.write("SAMP:COUN %d"%sample_num);
        self.write("TRIG:COUN 1");
        self.write("INIT");
    def display(self,cmdstr="READY"):
        self.write("DISP ON");
        self.write("DISP:TEXT \"%s\""%cmdstr);

    def disp_clr(self):
        self.write("DISP:TEXT:CLE");


if __name__=="__main__":
#def measure_current(typ=0,sample_rate=0.001,meastype='CURR',sample_num=512,maxvalue=0.3):
    
    while True:
	typ=1
	sample_rate=0.002
	precision_time = 1.6 #1.6ms
	meastype='CURR'
	sample_num=512
	maxvalue=0.4
	
	warn_point_value = 0
	
	import matplotlib.pyplot as plt
	
	import numpy as np
	
	#mydm = instrument.dm.dm()
	mydm=dm('dm2');
	#20
	I_sum=0;
	if typ==0:
	    mydm.start(sample_num=sample_num,meastype=meastype,maxvalue=maxvalue,res=1E-6)
	else:
	    mydm.start_rate(sample_num=sample_num,meastype=meastype,maxvalue=maxvalue,sample_rate=sample_rate)
	    
	time.sleep(sample_num*sample_rate)
	
	res = mydm.getcurve()
	# A to mA
	#print res[:10]
	#res=[(x-0.002) for x in res]
	for index, subres in enumerate(res):
	    res[index] = subres*1000
	    I_sum=I_sum+res[index]
	    
        I_sum=0
	for idx in range(len(res)):
	    if res[idx] < 0:
		#res[idx] = 0	
		res[idx] = res[idx] 
	    res[idx] = res[idx]
	    I_sum=I_sum+res[idx]	
	print "ave: %d" % (I_sum/len(res))
	###############################################################    
	itmp = 0;
	for idx in range(len(res)):
	    if res[idx]>0.5:
		itmp = idx
		break
	#res = res[(itmp-5):]
	
	itmp2=len(res)
	for idx in range(100,len(res)):
	    if res[idx]<0.5:
		itmp2=idx+2
		break
	#res = res[:itmp2]
	###############################################################	
	print "====================="
	print "res"
	print res
	print "====================="
	###################################################################
	#find out max and min
	res_max = res[0]
	res_min = res[0]
	for idx in range(len(res)):
	    if res_max < res[idx]:
		res_max = res[idx]
	    if res_min > res[idx]:
	        res_min = res[idx]
	a = (res_max+res_min)/2     #median
	b = I_sum/len(res) - res_min #avreage
	if warn_point_value == 0:
	    if a<=b :
		warn_point_value = a #select min
	    else:
		warn_point_value = b 
	    print "warn_point_value:",warn_point_value	
	
	#find out warn point    
	warn_point_x = [0]
	warn_point_y = [0]
	res_old = 0
	for idx in range(len(res)):
	    if abs(res[idx]-res_old) >= warn_point_value:
		if warn_point_x[-1] < (idx-1):
		    warn_point_x.append(idx-1)
		    warn_point_x.append(idx)
		    warn_point_y.append(res_old)
		    warn_point_y.append(res[idx])
	    res_old = res[idx]
	print "warn_point_x",warn_point_x
	print "warn_point_y",warn_point_y
	
	#integration
	integrat_val = [0]
	integrat_sum = 0
	
	for idx in range(1, len(warn_point_x)):
	    if idx%4 == 0:
		integrat_sum = 0
		for num in range(warn_point_x[idx-3], warn_point_x[idx]):
		    integrat_sum = integrat_sum + res[num]
		    
		integrat_val.append(integrat_sum)
		print "integrat*1.6 = %.1f"%(integrat_sum*precision_time)
		print "time*1.6 = %.1f"%((warn_point_x[idx]-warn_point_x[idx-3])*precision_time)
		print "range:x:%d-"%warn_point_x[idx-3],"x:%d\n"%warn_point_x[idx]
	 
	###################################################################	
	
	# plot
	x = np.array(range(0, len(res)));
	y = np.array(res);
	plt.figure(figsize=(10,10));
	plt.grid(True)
	plot1 = plt.plot(x,y,label="%s"%meastype,color="blue",linewidth=2);
	
	str_text = ''
	str_x = ''
	str_y = ''
	#for idx in range(len(warn_point_x)):
	    #str_x ='x:%d\n'%warn_point_x[idx]
	    #str_y ='y:%.1f'%warn_point_y[idx]
	    #str_text = str_x+str_y
	
	    #plt.plot(warn_point_x[idx],warn_point_y[idx],'ro',label="point")
	    #plt.annotate(str_text,color='red', xy=(warn_point_x[idx],warn_point_y[idx]), xytext=(warn_point_x[idx],warn_point_y[idx]+0.1))

    ##    plt.xlabel("%s"%xlabel);
    ##    plt.ylabel("%s"%ylabel);
    ##    lmt=getlmt(y);
        ##plt.ylim(lmt[0],lmt[1])
       ## plt.ylim(130,180);
        title = "Average:%.2f,"%(I_sum/len(res)) + "Sample rate:%.3f"%sample_rate
        plt.title(title);
	#plt.legend(plot1);
	plt.show();    
	raw_input("again?")
	