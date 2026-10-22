import java.util.NoSuchElementException;
import java.util.ArrayList;

import java.util.Scanner;
import java.io.IOException;
import java.lang.reflect.Array;
import java.io.File;
import java.net.Socket;

import java.net.*;
import java.io.*;

class oracle_node {

    private int num_virtual_nodes;

    private int server_port;

    // msg formats
    private int CONNECT_MSG_SIZE = 6;

    // used to listen to incoming connect requests
    private ServerSocket serverSocket;
    // the post connection tcp sockets
    private ArrayList<VirtualNode> virtual_nodes;

    // network layout
    private ArrayList<ArrayList<Edge>> adj;
    private String config_file_path;

    public oracle_node(String file_path, int port){
        
        this.num_virtual_nodes = -1;
        
        this.server_port = port;
        
        try{
            serverSocket = new ServerSocket(this.server_port);
        } catch (IOException e){
            e.printStackTrace();
            System.err.println("error in starting server socket, exiting");
            System.exit(1);
        }

        this.virtual_nodes = new ArrayList<VirtualNode>();

        this.adj = new ArrayList<ArrayList<Edge>>();
        this.config_file_path = file_path;

    }

    // returns only when all N connections are up, this is called periodically
    public void connect(){

        for(int i = 0; i < num_virtual_nodes; i++){
            // check if connection already exists
            if( ! virtual_nodes.get(i).check_connection()) {
                // establishing a fresh connection
                virtual_nodes.get(i).accept_connection(serverSocket);
            }
        }
    }

    public void process_connect_msg(){
        
        for(int i = 0; i < num_virtual_nodes; i++){
        // for(int i = 0; i < num_active_connections; i++){

            VirtualNode curr_vn = virtual_nodes.get(i);
            while(true){

                InputStream input = null;
                try {
                    input = curr_vn.socket.getInputStream();
                    
                    int bytes_avail = input.available();
                    if(bytes_avail >= CONNECT_MSG_SIZE){
                        // a msg present
                        byte[] b = null;
                        // TODDO handle case of partial message sitting in the buffer
                        b = input.readNBytes(CONNECT_MSG_SIZE);
                        b.toString();
                        System.out.print("got msg: ");
                        for(byte byt: b){
                            System.out.print(Byte.toUnsignedInt(byt) + " ");
                        }
                        System.out.print("\n");

                        System.arraycopy(b, 0, curr_vn.ip, 0, 4);
                        System.arraycopy(b, 4, curr_vn.port, 0, 2);
                        
                        break;
                    }
                    else{
                        System.out.println("no new message for VN: " + i);
                        Thread.sleep(2000);
                    }
                    
                } catch (Exception e){
                    System.out.println(e);
                    System.out.println(e.getMessage());
                    System.exit(1);
                }
            }
        }
    }

    public void send_link_states(){
        for(int i = 0; i < num_virtual_nodes; i++){
            virtual_nodes.get(i).send_link_state();
        }
    }

    // updates the adj list,  on exception
    public boolean read_config(){
        Scanner scanner = null;
        int row_num = 0;
        ArrayList<ArrayList<Edge>> new_adj = new ArrayList<ArrayList<Edge>>();
        int new_n = 0;

        // System.out.println("\nreading config");
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
                ArrayList<Integer> matrix_row = new ArrayList<Integer>();
                for(String s: digits_hopefully){
                    if(!s.matches("\\d+")){
                        // invalid line, non empty, does not start with "#", does not start with a digit
                        System.out.println("\n invalid line found: " + line);
                        System.out.println("graph remains unchanged");
                        
                        // do not update adj
                        if(scanner != null) scanner.close();
                        return false;
                    }
                    
                    // ok a number is found
                    matrix_row.add(Integer.valueOf(s));
                }
                
                // line was only numbers, good
                // update the class fields
                if(row_num == 0){
                    new_n = matrix_row.size()+1;
                    assert(adj != null);
                    for(int i=0; i<new_n; i++){
                        new_adj.add(new ArrayList<Edge>());
                    }
                    for(int i = 0; i < new_n; i++){
                        this.virtual_nodes.add(new VirtualNode(i, (char)('A'+i), null));
                    }
                } 
                
                if(row_num >= new_n-1) continue;

                int col_num = row_num+1;
                assert(matrix_row.size() == new_n-row_num-1);
                for(Integer d: matrix_row){
                    new_adj.get(row_num).add(new Edge(virtual_nodes.get(col_num), d));
                    if(col_num != row_num) new_adj.get(col_num).add(new Edge(virtual_nodes.get(row_num), d));
                    col_num++;
                }

                row_num++;
            }

            if(row_num != new_n-1){
                System.out.println("\nnot enough matrix rows");
                System.out.println("graph remains unchanged");
                        
                // do not update adj
                if(scanner != null) scanner.close();
                return false;
            }

            if(num_virtual_nodes == -1){
                num_virtual_nodes = new_n;
            }
            else assert(num_virtual_nodes == new_n);

            boolean changed = diff_adj(adj, new_adj);
            if(changed){
                System.out.println("\ngraph changed");
                adj = new_adj;

            }
            else{
                // System.out.println("graph remains unchanged");
                adj = new_adj;
                
            }
            
            if(scanner != null) scanner.close();
            return changed;

        } catch (Exception e){
            System.out.println("scanner exception\n");
            System.out.print(e);
            return false;
        }
    }

    public boolean diff_adj(ArrayList<ArrayList<Edge>> adj1, ArrayList<ArrayList<Edge>> adj2){
        
        if(adj1.size() != adj2.size()) return true;

        int n = adj1.size();
        for(int i = 0; i < n; i++){
            ArrayList<Edge> vec1 = adj1.get(i);
            ArrayList<Edge> vec2 = adj2.get(i);

            if(vec1.size() != vec2.size()) return true;
            
            for(int j = 0; j < vec1.size(); j++){
                if(vec1.get(j).dst.name != vec2.get(j).dst.name) return true;
                if(vec1.get(j).weight != vec2.get(j).weight) return true;
            }
        }

        return false;
    }

    public void printadj(){
        System.out.println(num_virtual_nodes);
        for(int i = 0; i < adj.size(); i++){
            System.out.println("row num: " + i);
            for(int j = 0; j < adj.get(i).size(); j++){
                System.out.print(adj.get(i).get(j).dst.id + " - " + adj.get(i).get(j).weight + ", ");
            }    
            System.out.print("\n");
        }    
    }

    private class Edge {
        VirtualNode dst;
        int weight;

        public Edge(VirtualNode dst, int weight){
            this.dst = dst;
            this.weight = weight;
        }
    }

    private class VirtualNode {

        public static final int max_retries = 4;

        char name;
        Socket socket;

        int id;
        byte[] ip;
        byte[] port;

        public VirtualNode(int id, char name, Socket socket){
            this.id = id;
            this.name = name;
            this.socket = socket;

            ip = new byte[4];
            port = new byte[2];
        }

        public boolean check_connection() {
            if(socket == null || socket.isClosed()) return false;
            else return true;
        }

        public boolean accept_connection(ServerSocket serverSocket){
            int attempt = 0;
            while (attempt < max_retries) {
                System.out.println("virtual node: " + name + ", trying to accept connection, retry: " + attempt);
                try{
                    socket = serverSocket.accept();
                    System.out.println("virtual node: " + name + ", connection established");
                    return true;
                } catch (Exception e) {
                    e.printStackTrace();
                    attempt++;
                }
            }
            
            System.out.println("virtual node: " + name + "max retries reached, exiting");
            System.exit(1);
            return false;
        }


        public void send_link_state(){
            
            int num_links = adj.get(id).size();
            
            int tuple_size = 11; // 1+4+2+4
            byte[] tuple = new byte[tuple_size];
            
            int LSP_offset = 4;
            byte[] LSP = new byte[4 + tuple_size*num_links];

            LSP[0] = (byte)(num_links       & 0xFF);
            LSP[1] = (byte)(num_links >>  8 & 0xFF);
            LSP[2] = (byte)(num_links >> 16 & 0xFF);
            LSP[3] = (byte)(num_links >> 24 & 0xFF);

            System.out.println("\nsending link state for: " + name);

            for(Edge e: adj.get(id)){
                // name + ip + port + cost(int)
                tuple[0] = ((byte)e.dst.name);
                

                byte[] dst_ip = e.dst.ip;
                assert(dst_ip != null);
                tuple[1] = dst_ip[0];
                tuple[2] = dst_ip[1];
                tuple[3] = dst_ip[2];
                tuple[4] = dst_ip[3];
                
                byte[] dst_port = e.dst.port;
                assert(dst_port != null);
                tuple[5] = dst_port[0];
                tuple[6] = dst_port[1];

                int cost = e.weight;
                tuple[7] = (byte)(cost >> 24 & 0xFF);
                tuple[8] = (byte)(cost >> 16 & 0xFF);
                tuple[9] = (byte)(cost >>  8 & 0xFF);
                tuple[10] = (byte)(cost       & 0xFF);
                
                System.arraycopy(tuple, 0, LSP, LSP_offset, tuple_size);
                LSP_offset += tuple_size;
            }

            for(byte byt: LSP){
                System.out.print(Byte.toUnsignedInt(byt) + " ");
            }
            System.out.print("\n");

            try{
                OutputStream output = socket.getOutputStream();
                output.write(LSP);
            } catch (IOException e){
                System.out.println("failed in writing to node " + name + " socket");
            }
        }
    }
}


public class oracle {

    public static void main(String[] args){
        
        oracle_node on = new oracle_node("config.txt", 5000);

        try {
        // read config file once
        on.read_config();
        on.printadj();
        
        // wait for all N CONNECT messages
        on.connect();
        on.process_connect_msg();
        
        on.send_link_states();

        // in a loop keep checking for user input, on input update adj resend the LINK STATE packets to all VNs
        while(true){
            boolean updated = on.read_config();

            if(updated){
                on.send_link_states();
            }

            Thread.sleep(1000);
        }
        } catch (Exception e) { e.printStackTrace();}
    }
}