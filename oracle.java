import java.util.List;
import java.util.NoSuchElementException;
import java.util.ArrayList;

import java.util.Scanner;
import java.io.IOException;
import java.io.File;
import java.net.Socket;

import java.net.*;
import java.io.*;

class oracle_node {

    private int n = -1;
    private class Edge {
        int dst;
        Double weight;

        public Edge(int dst, Double weight){
            this.dst = dst;
            this.weight = weight;
        }
    }

    private int port = 5000;
    private int num_active_connections = 0;
    private ServerSocket serverSocket;
    private ArrayList<Socket> VN_sockets = new ArrayList<Socket>();

    private ArrayList<ArrayList<Edge>> adj = new ArrayList<ArrayList<Edge>>();
    private String config_file_path;

    public oracle_node(String file_path){
        
        this.config_file_path = file_path;
        try{
            serverSocket = new ServerSocket(port);
        } catch (IOException e){
            System.out.println(e);
            System.out.println(e.getMessage());
            System.exit(1);
        }
    }

    // returns only when all N connections are up, this is called periodically
    public void connect(){
        Socket clientsocket = null;
        for(int i = 0; i < n; i++){
            try{
                num_active_connections++;
                clientsocket = serverSocket.accept();
                System.out.println("VN accepted: " + num_active_connections);
                VN_sockets.add(clientsocket);
            } catch (Exception e){
                System.out.println(e);
                System.out.println(e.getMessage());
                System.exit(1);
            }
        }
    }

    public void process_pending_msgs(){
        
        for(int i = 0; i < num_active_connections; i++){
            InputStream input = null;
            try {
                input = VN_sockets.get(i).getInputStream();
                
                int bytes_avail = input.available();
                if(bytes_avail != 0){
                    // a msg present
                    byte[] b = null;
                    // TODDO handle case of partial message sitting in the buffer
                    b = input.readNBytes(bytes_avail);
                    b.toString();
                    System.out.print("got msg: ");
                    System.out.println(b);
                }
                else{
                    System.out.println("no new message for VN: " + i);
                }
            
            } catch (Exception e){
                System.out.println(e);
                System.out.println(e.getMessage());
                System.exit(1);
            }
        }
    }

    // updates the adj list,  on exception
    public void read_config(){
        Scanner scanner = null;
        int row_num = 1;
        ArrayList<ArrayList<Edge>> new_adj = new ArrayList<ArrayList<Edge>>();
        int new_n = 0;
        try{
            scanner = new Scanner(new File(config_file_path));
            String line, stripped_line;
            while(true){
                try{
                    line = scanner.nextLine();
                } catch (NoSuchElementException e){
                    break;
                }

                stripped_line = line.strip();
                if(stripped_line.isBlank() || stripped_line.startsWith("#")) continue;

                String[] digits_hopefully = stripped_line.split("\\s+");
                ArrayList<Double> matrix_row = new ArrayList<Double>();
                for(String s: digits_hopefully){
                    if(!s.matches("\\d+")){
                        // invalid line, non empty, does not start with "#", does not start with a digit
                        System.out.println("invalid line found: " + line);
                        System.out.println("graph remains unchanged");
                        
                        // do not update adj
                        return;
                    }

                    // ok a number is found
                    matrix_row.add(Double.valueOf(s));
                }

                // line was only numbers, good
                // update the class fields
                if(row_num == 1){
                    new_n = matrix_row.size();
                    assert(adj != null);
                    for(int i=0; i<new_n+1; i++){
                        new_adj.add(new ArrayList<Edge>());
                    }
                } 
                

                int col_num = row_num;
                assert(matrix_row.size() == new_n-row_num+1);
                for(Double d: matrix_row){
                    new_adj.get(row_num).add(new Edge(col_num, d));
                    if(col_num != row_num) new_adj.get(col_num).add(new Edge(row_num, d));
                    col_num++;
                }

                row_num++;
            }

            if(row_num != new_n+1){
                System.out.println("not enough matrix rows");
                System.out.println("graph remains unchanged");
                        
                // do not update adj
                return;
            }

            if(n == -1){
                n = new_n;
            }
            else assert(n == new_n);

            if(adj == new_adj){
                System.out.println("graph remains unchanged");
            }

            adj = new_adj;
            

        } catch (Exception e){
            System.out.println("scanner exception\n");
            System.out.print(e);
        }
        finally{
            if(scanner != null) scanner.close();
        }
    }

    public void printadj(){
        System.out.println(n);
        for(int i = 1; i < adj.size(); i++){
            System.out.println("row num: " + i);
            for(int j = 0; j < adj.get(i).size(); j++){
                System.out.print(adj.get(i).get(j).dst + " - " + adj.get(i).get(j).weight + ", ");
            }    
            System.out.print("\n");
        }    
    }
}


public class oracle {

    public static void main(String[] args){
        oracle_node on = new oracle_node("config.txt");

        // read config file once
        on.read_config();
        on.printadj();
        
        // wait for all N CONNECT messages
        on.connect();
        on.process_pending_msgs();
        
        // in a loop keep checking for user input, on input update adj resend the LINK STATE packets to all VNs

    }
}