BEGIN{t1=0;totalBytes=0;tb=0}
{
	
	if($2-t1>=0.01)
	{
		printf("%f %f\n",$2,totalBytes*100);
		totalBytes=0;
		t1=$2;
	}

	totalBytes += $6;
	tb += $6;
}
END{printf("Total bytes = %f",tb);}
