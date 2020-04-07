 module ATA(reset, cs5, moe, mwe, clk, intrq, 
           exprdy, cs0, cs1, eint, dior, diow, rw, oe,
		   a0, a1, a2, a3, da0, da1, da2, iordy);
		   
           // Input declarations
		   input reset;
		   input cs5;
		   input moe;
		   input mwe;
		   input a3, a2, a1, a0;
		   input clk;
		   input intrq;
		   input iordy;

		   // Output declarations
		   output rw;
		   output oe;
		   output exprdy;
		   output cs0;
		   output cs1;
		   output eint;
		   output da2, da1, da0;
		   output dior;
		   output diow;

           // Internal Variables
		   reg tmp_exprdy;
		   reg tmp_cs0;
		   reg tmp_cs1;
		   reg tmp_dior;
		   reg tmp_diow;
		   reg tmp_rw;
		   reg [5:0] count;
		   reg [2:0] state;

		   parameter[2:0]
		            IDLE = 0,
		            CS_ASSERTED = 1,
		            READ = 2,
					NORMAL_READ = 3,
					IORDY_READ = 4,
					WRITE = 5,
					NORMAL_WRITE = 6,
					IORDY_WRITE = 7;

           // Update the state of all the external ouput pins.
		   assign da2 = a2;
		   assign da1 = a1;
		   assign da0 = a0;
		   assign rw = tmp_rw;
		   assign oe = tmp_cs0 & tmp_cs1;
		   assign eint = !intrq;
		   assign exprdy = tmp_exprdy;
		   assign cs0 = tmp_cs0;
		   assign cs1 = tmp_cs1;
		   assign dior = tmp_dior;
		   assign diow = tmp_diow;
			
		   always @(posedge clk or negedge reset)
		   begin

		    // Handle a reset by initializing the state machine.
		    if(reset == 1'b 0)
	         begin
			  tmp_exprdy <= 1'b 1;
		      tmp_cs0 <= 1'b 1;
		      tmp_cs1 <= 1'b 1;
		      tmp_dior <= 1'b 1;
		      tmp_diow <= 1'b 1;
			  tmp_rw <= 1'b 1;
		      count <= 6'b 000000;
			  state <= IDLE;
			 end
            else 
		     begin

	        // Implementation of the state machine.
		    case(state) // warp parallel_case
		     IDLE: begin
			 
			 // Check for the assertion of CS5. Assert the appropriate
			 // ATA chip select, and move to the next state. 
		     if(cs5 == 1'b 0) 
			  begin
		       if(a3 == 1'b 0)
				begin
			     tmp_cs0 <= 1'b 0;
				 tmp_exprdy <= 1'b 0;
				 state <= CS_ASSERTED;
			    end
			   else 
				begin
			     tmp_cs1 <= 1'b 0;
				 tmp_exprdy <= 1'b 0;
				 state <= CS_ASSERTED;
			    end
              end
			 end
             
			 // Wait for the assertion of /MOE or /MWE to determine if a
			 // read or write access is to take place.
			 CS_ASSERTED: begin
             if(moe == 1'b 0)
			  begin
			   state <= READ;
			   tmp_rw <= 1'b 0;
			  end
			 if(mwe == 1'b 0)
 			  begin
			   state <= WRITE;
			   tmp_rw <= 1'b 1;
			  end
			 end
			 
			 // If a read access is taking place, assert /DIOR and wait to 
			 // determine if IORDY will be asserted for the cycle.
			 READ: begin
			 count <= count + 1;

			 // Activate dior on a count of two to comply with the 
			 // chip select to /DIOR timing spec.
			 if(count == 6'b 000010) 
			  begin
 			   tmp_dior <= 1'b 0;
			  end

			 // Check for the negation of IORDY two clocks after the
			 // assertion of /DIOR per the ATA spec. 
             if(count == 6'b 000100)
			  begin
			  
 			   // If IORDY is deasserted, perform an IORDY controlled read
			   // access, otherwise perform a normal read.
			   if(iordy == 1'b 0)
			     state <= IORDY_READ;
			   else
			     state <= NORMAL_READ;
              end
             end

			 // Performs a normal ATA read access.
			 NORMAL_READ: begin
			  count <= count + 1;
			  if(count == 6'b 010000) 
			   begin
			    tmp_exprdy <= 1'b 1;
			   end
			  if(count == 6'b 010010)
			   begin
			    tmp_dior <= 1'b 1;
			   end
			  if(count == 6'b 010011) 
               begin
			    tmp_cs0 <= 1'b 1;
			    tmp_cs1 <= 1'b 1;
				tmp_rw <= 1'b 1;
			    count <= 6'b 000000;
			    state <= IDLE; 
			   end
 			 end

			 // Perform an IORDY read access per the ATA timing spec.
			 IORDY_READ: begin
			  count <= count + 1; 
              
			  // Wait for the assertion of IORDY or an IORDY timeout.
			  if((iordy == 1'b 1) || (tmp_exprdy == 1'b 1) || (count >= 6'b 101101))
			   begin
			    if(tmp_exprdy == 1'b 0)
				 begin
				  tmp_exprdy <= 1'b 1;
				  count <= 6'b 000000;
			     end

			    // Deassert /DIOR a little late to allow the EP72xx 
				// to read data.
                else
				 begin
			      if(count == 6'b 000010)
			       begin
			        tmp_dior <= 1'b 1;
			       end
				  if(count == 6'b 000011)
				   begin
				    tmp_cs0 <= 1'b 1;
			        tmp_cs1 <= 1'b 1;
					tmp_rw <= 1'b 1;
			        count <= 6'b 000000;
			        state <= IDLE; 
                   end
				end
             end
			end

			 // If a write access is taking place, assert /DIOW and wait to 
			 // determine if IORDY will be asserted for the cycle.
			 WRITE: begin
			 count <= count + 1;

			 // Activate /DIOW on a count of two to comply with the 
			 // chip select to /DIOW timing spec.
			 if(count == 6'b 000010) 
			  begin
 			   tmp_diow <= 1'b 0;
			  end

			 // Check for the negation of IORDY two clocks after the
			 // assertion of /DIOW per the ATA spec. 
             if(count == 6'b 000100)
			  begin
			  
 			   // If IORDY is deasserted, perform an IORDY controlled write
			   // access, otherwise perform a normal write.
			   if(iordy == 1'b 0)
			     state <= IORDY_WRITE;
			   else
			     state <= NORMAL_WRITE;
              end
             end

             // Perform a normal write access.
             NORMAL_WRITE: begin
              count <= count + 1;
			  if(count == 6'b 010101) 
			   begin
			    tmp_exprdy <= 1'b1;
			    tmp_diow <= 1'b1;
			   end
			  if(count == 6'b 010111) 
               begin
			    tmp_cs0 <= 1'b 1;
			    tmp_cs1 <= 1'b 1;
			    count <= 6'b 000000;
			    state <= IDLE; 
			   end
 			 end

			 // Perform an IORDY controlled write access per the ATA timing spec.
			 IORDY_WRITE: begin
			  count <= count + 1;

			  // Wait for the assertion of IORDY or an IORDY timeout to terminate
 			  // the cycle.
			  if((iordy == 1'b 1) || (tmp_exprdy == 1'b 1)||(count >= 6'b 101101))
			   begin
                
  			    // Terminate the IORDY controlled write access.
				if(tmp_exprdy == 1'b 0)
				 begin
				  tmp_exprdy <= 1'b 1;

				  // Deassert /DIOW at this time to meet ATA write data 
				  // hold spec.
				  tmp_diow <= 1'b 1;
     			  count <= 6'b 000000;
				 end

			    // Complete the write access.
			    else
			     begin
			      if(count == 6'b 000001)
			       begin
				    tmp_cs0 <= 1'b 1;
			        tmp_cs1 <= 1'b 1;
			        count <= 6'b 000000;
			        state <= IDLE; 
                   end
			     end
                end
			  end
		   endcase
		   end
		   end
	         		  
endmodule


