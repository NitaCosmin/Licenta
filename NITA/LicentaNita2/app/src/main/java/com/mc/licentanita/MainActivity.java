package com.mc.licentanita;

import android.Manifest;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.activity.EdgeToEdge;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.OptIn;
import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.media3.common.util.UnstableApi;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Set;
import java.util.UUID;

import okhttp3.Call;
import okhttp3.Callback;
import okhttp3.FormBody;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";

    // Replace with your actual UUID
    private static final UUID MY_UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");

    private static final int REQUEST_BLUETOOTH_SCAN_PERMISSION = 1;
    private static final int REQUEST_BLUETOOTH_CONNECT_PERMISSION = 2;

    private BluetoothAdapter bluetoothAdapter;
    private BluetoothSocket socket;
    private InputStream inputStream;
    private OutputStream outputStream;
    private ArrayAdapter<String> deviceListAdapter;
    private EditText messageEditText;
    private Button connectButton;
    private TextView terminalTextView;
    private BluetoothDevice connectedDevice;
    private final ArrayList<BluetoothDevice> discoveredDevices = new ArrayList<>();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        EdgeToEdge.enable(this);

        // Initialize views
        terminalTextView = findViewById(R.id.terminalTextView);
        connectButton = findViewById(R.id.connectButton);
        ListView deviceListView = findViewById(R.id.deviceListView);
        messageEditText = findViewById(R.id.messageEditText);
        Button sendButton = findViewById(R.id.sendButton);

        deviceListAdapter = new ArrayAdapter<>(this, android.R.layout.simple_list_item_1);
        deviceListView.setAdapter(deviceListAdapter);

        // Set onClickListener for deviceListView
        deviceListView.setOnItemClickListener((parent, view, position, id) -> {
            BluetoothDevice device = discoveredDevices.get(position);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                connectToSelectedDevice(device);
            }
        });

        // Request Bluetooth permissions
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            requestBluetoothPermissions();
        }

        connectButton.setOnClickListener(v -> {
            if (socket == null || !socket.isConnected()) {
                discoverBluetoothDevices();
            } else {
                disconnectBluetooth();
            }
        });

        sendButton.setOnClickListener(v -> {
            if (socket != null && socket.isConnected()) {
                String message = messageEditText.getText().toString();
                if (message.isEmpty()) {
                    Toast.makeText(this, "Message cannot be empty.", Toast.LENGTH_SHORT).show();
                    return;
                }
                Log.d(TAG, "Sending message: " + message);
                terminalTextView.append("You: " + message + " \n");
                messageEditText.setText("");
                // Send the message to the connected device
                sendMessage(message);
                Log.d(TAG, "Message sent.");

            } else {
                Toast.makeText(this, "Not connected to a device.", Toast.LENGTH_SHORT).show();
            }
        });
    }

    @RequiresApi(api = Build.VERSION_CODES.S)
    private void requestBluetoothPermissions() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.BLUETOOTH_CONNECT},
                    REQUEST_BLUETOOTH_CONNECT_PERMISSION);
        } else {
            Log.d(TAG, "Bluetooth connect permission already granted.");
        }
    }

    private void discoverBluetoothDevices() {
        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter(); // Initialize here
        if (bluetoothAdapter == null) {
            // Device doesn't support Bluetooth
            Toast.makeText(this, "Bluetooth is not supported on this device.", Toast.LENGTH_SHORT).show();
            Log.e(TAG, "Bluetooth adapter is null. Device does not support Bluetooth.");
            return;
        }

        if (!bluetoothAdapter.isEnabled()) {
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableBtIntent, REQUEST_BLUETOOTH_SCAN_PERMISSION);
            Log.d(TAG, "Bluetooth adapter is not enabled. Requesting to enable.");
        } else {
            Log.d(TAG, "Bluetooth adapter is already enabled. Starting device discovery.");
            deviceListAdapter.clear();
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                if (ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
                    ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.BLUETOOTH_SCAN}, REQUEST_BLUETOOTH_SCAN_PERMISSION);
                    Log.d(TAG, "Bluetooth scan permission not granted. Requesting permission.");
                    return; // Wait for permission result
                }
            }

            Set<BluetoothDevice> pairedDevices = bluetoothAdapter.getBondedDevices();
            for (BluetoothDevice device : pairedDevices) {
                discoveredDevices.add(device);
                deviceListAdapter.add(device.getName() + "\n" + device.getAddress());
                Log.d(TAG, "Paired device found: " + device.getName() + " - " + device.getAddress());
            }
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_BLUETOOTH_SCAN_PERMISSION) {
            if (resultCode == RESULT_OK) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                    if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED ||
                            ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED ||
                            ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {

                        ActivityCompat.requestPermissions(this,
                                new String[]{Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_COARSE_LOCATION}, REQUEST_BLUETOOTH_SCAN_PERMISSION);
                        Log.d(TAG, "Bluetooth scan permission or location permissions not granted. Requesting permissions.");
                    } else {
                        // Permissions already granted, proceed with discovery
                        discoverBluetoothDevices();
                        Log.d(TAG, "Bluetooth scan permission and location permissions already granted. Starting device discovery.");
                    }
                }
            } else {
                Toast.makeText(this, "Bluetooth enable request denied.", Toast.LENGTH_SHORT).show();
                Log.d(TAG, "Bluetooth enable request denied.");
            }
        }
    }

    @RequiresApi(api = Build.VERSION_CODES.S)
    private void connectToSelectedDevice(BluetoothDevice device) {
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
            // Permission is not granted, request it
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.BLUETOOTH_SCAN},
                    REQUEST_BLUETOOTH_SCAN_PERMISSION); // Use a unique request code
        } else {
            if (bluetoothAdapter.isDiscovering()) {
                bluetoothAdapter.cancelDiscovery();
                Log.d(TAG, "Canceling ongoing discovery before connecting to device.");
            }
            connectToDevice(device);
        }
    }

    private void connectToDevice(BluetoothDevice device) {
        new Thread(() -> {
            try {
                if (ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                    // TODO: Handle permission request
                    Log.e(TAG, "Bluetooth connect permission not granted. Cannot connect to device.");
                    return;
                }
                socket = device.createRfcommSocketToServiceRecord(MY_UUID);
                socket.connect();
                connectedDevice = device;
                runOnUiThread(() -> {
                    connectButton.setText("Disconnect");
                    startDataTransfer();
                });
                Log.d(TAG, "Connected to device: " + device.getName() + " - " + device.getAddress());
            } catch (IOException e) {
                runOnUiThread(() -> {
                    Toast.makeText(this, "Connection failed: " + e.getMessage(), Toast.LENGTH_SHORT).show();
                });
                socket = null;
                Log.e(TAG, "Connection failed: " + e.getMessage());
            }
        }).start();
    }

    private void startDataTransfer() {
        try {
            inputStream = socket.getInputStream();
            outputStream = socket.getOutputStream();

            // Receive data (in a separate thread)
            new Thread(() -> {
                byte[] buffer = new byte[1024];
                int bytes;
                try {
                    while (socket != null && socket.isConnected()) {
                        bytes = inputStream.read(buffer);if (bytes > 0) {
                            final String receivedData = new String(buffer, 0, bytes);
                            runOnUiThread(() -> {
                                // Display received data in the terminal
                                Log.d(TAG, "Received data: " + receivedData);
                                terminalTextView.append("Remote: " + receivedData + "\n");

                                // Send data to endpoint
                                try {
                                    sendDataToEndpoint("http://79.112.179.227/api/add_device", receivedData);
                                }
                                catch (Exception e) {
                                    Log.e(TAG, "Error sending data to endpoint: " + e.getMessage());
                                    runOnUiThread(() -> {
                                        Toast.makeText(this, "Error sending data to endpoint:" + e.getMessage(), Toast.LENGTH_SHORT).show();
                                        disconnectBluetooth(); // Or implement your disconnection logic
                                    });
                                }
                            });
                        }
                    }
                } catch (IOException e) {
                    // Handle read errors
                    Log.e(TAG, "Error reading data: " + e.getMessage());
                }
            }).start();

        } catch (IOException e) {
            Toast.makeText(this, "Error starting data transfer: " + e.getMessage(), Toast.LENGTH_SHORT).show();
            Log.e(TAG, "Error starting data transfer: " + e.getMessage());
        }
    }

    private void sendDataToEndpoint(String endpointUrl, String data) {
        OkHttpClient client = new OkHttpClient();

        // Extract name and domain from received data
        String name = data.substring(data.indexOf("BTname:") + 7, data.indexOf(" IP:")).trim();
        String domain = data.substring(data.indexOf("IP:") + 3).trim();

        // Create form body
        RequestBody body = new FormBody.Builder()
                .add("name", name)
                .add("domain", domain)
                .build();

        // Create request
        Request request = new Request.Builder()
                .url(endpointUrl)
                .post(body)
                .build();

        // Send request
        client.newCall(request).enqueue(new Callback() {
            @Override
            public void onFailure(@NonNull Call call, @NonNull IOException e) {
                Log.e(TAG, "Error sending data to endpoint: " + e.getMessage());
            }

            @Override
            public void onResponse(@NonNull Call call, @NonNull Response response) throws IOException {
                if (response.isSuccessful()) {
                    Log.d(TAG, "Data sent to endpoint successfully.");
                } else {
                    Log.e(TAG, "Error sending data to endpoint: " + response.code());
                }
            }
        });
    }

    @OptIn(markerClass = UnstableApi.class)
    private void sendMessage(String message) {
        new Thread(() -> {
            if (socket != null && socket.isConnected()) { // Check connection before sending
                try {
                    outputStream.write(message.getBytes());
                } catch (IOException e) {
                    androidx.media3.common.util.Log.e(TAG, "Error sending message: " + e.getMessage());
                    // Handle the error, e.g., close the socket and notify the user
                    runOnUiThread(() -> {
                        Toast.makeText(this, "Connection lost!", Toast.LENGTH_SHORT).show();
                        disconnectBluetooth(); // Or implement your disconnection logic
                    });
                }
            } else {
                androidx.media3.common.util.Log.e(TAG, "Cannot send message: Socket is not connected.");
                // Notify the user that the connection is lost
            }
        }).start();
    }


    private void disconnectBluetooth() {
        if (socket != null) {
            try {
                socket.close();
                inputStream.close();
                outputStream.close();
                socket = null;
                inputStream = null;
                outputStream = null;
                connectButton.setText("Connect");
                terminalTextView.append("Disconnected.\n");
                Log.d(TAG, "Disconnected from Bluetooth device.");
            } catch (IOException e) {
                Toast.makeText(this, "Error disconnecting: " + e.getMessage(), Toast.LENGTH_SHORT).show();
                Log.e(TAG, "Error disconnecting: " + e.getMessage());
            }
        }
    }

    @OptIn(markerClass = UnstableApi.class)
    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == REQUEST_BLUETOOTH_SCAN_PERMISSION) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                // Permission granted, proceed with connection
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                    if (ActivityCompat.checkSelfPermission(this,Manifest.permission.BLUETOOTH_SCAN) == PackageManager.PERMISSION_GRANTED) {
                        if (bluetoothAdapter.isDiscovering()) {
                            bluetoothAdapter.cancelDiscovery();
                        }
                    } else {
                        // Handle the case where BLUETOOTH_SCAN permission is not granted
                        // You might want to request the permission here or inform the user
                        Log.e(TAG, "BLUETOOTH_SCAN permission not granted. Cannot cancel discovery.");
                    }
                } else {
                    // For older Android versions, no need for explicit permission check
                    if (bluetoothAdapter.isDiscovering()) {
                        bluetoothAdapter.cancelDiscovery();
                    }
                }
                // ... (rest of your connectToSelectedDevice method, continuing from where you left off)
            } else {
                // Permission denied, handle accordingly (e.g., show amessage)
                androidx.media3.common.util.Log.e(TAG, "Bluetooth scan permission denied. Cannot connect to device.");
                Toast.makeText(this, "Bluetooth scan permission denied.", Toast.LENGTH_SHORT).show();
            }
        }
    }
    @Override
    protected void onDestroy() {
        super.onDestroy();
        disconnectBluetooth();
        Log.d(TAG, "MainActivity destroyed.");
    }
}
