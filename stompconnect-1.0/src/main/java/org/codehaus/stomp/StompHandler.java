/**
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.codehaus.stomp;

import java.io.IOException;
import java.net.URISyntaxException;

import javax.jms.JMSException;

/**
 * Represents a handler of Stomp frames
 * 
 * @version $Revision: 65 $
 */
public interface StompHandler {
    void onStompFrame(StompFrame frame) throws IOException;

    void onException(Exception e);

    /**
     * Called to close the handler
     * 
     * @throws InterruptedException
     * @throws IOException
     * @throws JMSException
     * @throws URISyntaxException
     */
    void close() throws InterruptedException, IOException, JMSException, URISyntaxException;
}
