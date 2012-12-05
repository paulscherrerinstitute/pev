%Little script to write a ramp in 3 IFC SMEM Memory
%Then to read back to verifiy contents
%Then to use master to get data from slave through VME

%Two ramp types to differentiate results

%Incremental ramp generation 32K element
ramp(1)=0;
for k=2:32768
    ramp(k)=ramp(k-1)+1;
end;


%Decremental ramp generation 32K element
ramp_inv(1)=32768;
for k=2:32768
    ramp_inv(k)=ramp_inv(k-1)-1;
end;

%close all;
figure(1);
subplot(3,3,1);plot(ramp);title('Data in IFC1');
subplot(3,3,2);plot(ramp);title('Data in IFC2');
subplot(3,3,3);plot(ramp_inv);title('Data in IFC3');

%Write in SMEMs
caput('MTEST-VME-KR841:SMEM-WAVEFORM-W',ramp);
caput('MTEST-VME-KR842:SMEM-WAVEFORM-W',ramp);
caput('MTEST-VME-KR843:SMEM-WAVEFORM-W',ramp_inv);


%Readback for verification
%caput('MTEST-VME-KR841:SMEM-ReadInitiator.PROC',1);
a= caget ('MTEST-VME-KR841:SMEM-DATA-R'); localread1=a.val;plot (localread1);
caput('MTEST-VME-KR842:SMEM-ReadInitiator.PROC',1);
a= caget ('MTEST-VME-KR842:SMEM-DATA-R'); localread2=a.val;plot (localread2);
caput('MTEST-VME-KR843:SMEM-ReadInitiator.PROC',1);
a= caget ('MTEST-VME-KR843:SMEM-DATA-R'); localread3=a.val;plot (localread3);

subplot(3,3,4);plot(localread1);title('Readback Data from IFC1');
subplot(3,3,5);plot(localread2);title('Readback Data from IFC2');
subplot(3,3,6);plot(localread3);title('Readback Data from IFC3');


%Readback using master and VME remote communication
%caput('MTEST-VME-KR841:SMEM-ReadInitiator.PROC',1);
a= caget ('MTEST-VME-KR841:SMEM-DATA-R'); remoteread1=a.val;plot (remoteread1);
a= caget ('MTEST-VME-KR841:REMOTE2-DATA-R'); remoteread2=a.val;plot (remoteread2);
a= caget ('MTEST-VME-KR841:REMOTE3-DATA-R'); remoteread3=a.val;plot (remoteread3);

subplot(3,3,7);plot(remoteread1);title('Readback Data from IFC1');
subplot(3,3,8);plot(remoteread2);title('Remote Data from IFC2');
subplot(3,3,9);plot(remoteread3);title('Remote Data from IFC3');

figure(2)
plot(remoteread3)
